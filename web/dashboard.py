#!/usr/bin/env python3
"""Read BBB IIO ADC0, exchange values with /dev/bbb_freq using ioctl,
then publish read-only HTTP telemetry. Educational LAN-only service.
"""

import fcntl                       # Linux ioctl userspace interface.
import json                        # Encode measurement results as JSON.
import os                          # Read optional service configuration.
from pathlib import Path           # Locate ADC channels and HTML files.
import struct                      # Pack and unpack 32-bit ioctl arguments.
import threading                   # Serialize the SET_ADC/GET_FREQ pair.
import time                        # Timestamp successful measurements.
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# These numbers match _IOW('F', 1, __u32) and _IOR('F', 2, __u32).
# Do not change them without updating the shared header and kernel module.
FREQ_IOCTL_SET_ADC = 0x40044601
FREQ_IOCTL_GET_FREQ = 0x80044602

DEVICE_PATH = os.environ.get('BBB_FREQ_DEVICE', '/dev/bbb_freq')
HTML_PATH = Path(os.environ.get('BBB_FREQ_HTML', Path(__file__).with_name('index.html')))
HOST = os.environ.get('BBB_FREQ_HOST', '0.0.0.0')   # LAN access; no authentication.
PORT = int(os.environ.get('BBB_FREQ_PORT', '8000'))
ADC_NAME = 'TI-am335x-adc'                      # Match the BBB's actual IIO device.
ADC_RAW_MAX = 4095                             # The AM335x ADC is 12-bit.

# The POT spans 0–1.8 V and the driver expects raw ADC counts 0–4095.
# In this POT test, no 3.3 V voltage-divider conversion is necessary.
ioctl_lock = threading.Lock()                  # One request at a time in this process.


def select_adc_path():
    """Find the AM335x IIO device rather than assuming iio:device0."""
    explicit = os.environ.get('BBB_FREQ_ADC_PATH')
    if explicit:                               # Override for special board setups.
        return Path(explicit)

    for name_file in sorted(Path('/sys/bus/iio/devices').glob('iio:device*/name')):
        device_name = name_file.read_text(encoding='ascii').strip()
        if device_name.startswith(ADC_NAME):   # Example: TI-am335x-adc.0.auto.
            return name_file.parent / 'in_voltage0_raw'

    raise FileNotFoundError('Cannot locate TI AM335x ADC0 IIO channel')


def read_measurement(adc_path):
    """Return live ADC and frequency, using the existing ioctl driver."""
    raw = int(adc_path.read_text(encoding='ascii').strip())
    if not 0 <= raw <= ADC_RAW_MAX:             # Do not send invalid raw counts.
        raise ValueError(f'Unexpected ADC count: {raw}')

    # Native-standard 32-bit unsigned integer is the ioctl ABI's __u32.
    adc_buffer = struct.pack('=I', raw)
    frequency_buffer = bytearray(4)            # Kernel copies the 4-byte result here.

    # Two separate ioctl commands use driver-global state. Serialize them here.
    # Another independent application could still interleave its own calls.
    with ioctl_lock:
        with open(DEVICE_PATH, 'rb+', buffering=0) as device:
            fcntl.ioctl(device.fileno(), FREQ_IOCTL_SET_ADC, adc_buffer)
            fcntl.ioctl(device.fileno(), FREQ_IOCTL_GET_FREQ, frequency_buffer, True)

    millihertz = struct.unpack('=I', frequency_buffer)[0]
    return {
        'adc_raw': raw,                       # Physical ADC reading (counts).
        'frequency_hz': millihertz / 1000.0,  # Converted frequency (hertz).
        'timestamp_ms': int(time.time() * 1000),
    }


class TelemetryHandler(BaseHTTPRequestHandler):
    """Serve one static HTML page and a single read-only JSON endpoint."""

    def respond(self, code, payload, content_type):
        self.send_response(code)
        self.send_header('Content-Type', content_type)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Length', str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path == '/':
            try:
                page = HTML_PATH.read_bytes()  # Fetch the packaged dashboard.
                self.respond(200, page, 'text/html; charset=utf-8')
            except OSError:
                self.respond(500, b'Dashboard HTML not found', 'text/plain; charset=utf-8')
            return

        if self.path == '/api/frequency':
            try:
                measurement = read_measurement(self.server.adc_path)
                response = json.dumps(measurement).encode('utf-8')
                self.respond(200, response, 'application/json')
            except (OSError, ValueError) as exc:
                # Give the browser an error without inventing a sensor reading.
                response = json.dumps({'error': str(exc)}).encode('utf-8')
                self.respond(503, response, 'application/json')
            return

        self.respond(404, b'Not found', 'text/plain; charset=utf-8')


def main():
    adc_path = select_adc_path()               # Fail early if ADC is unavailable.
    if not Path(DEVICE_PATH).exists():
        raise FileNotFoundError(f'Kernel character device missing: {DEVICE_PATH}')

    server = ThreadingHTTPServer((HOST, PORT), TelemetryHandler)
    server.adc_path = adc_path                 # Store the chosen IIO path on server.
    print(f'ADC={adc_path}, device={DEVICE_PATH}', flush=True)
    print(f'Dashboard: http://{HOST}:{PORT}/', flush=True)
    try:
        server.serve_forever(poll_interval=0.5)
    except KeyboardInterrupt:
        pass                                  # Stop with Ctrl+C in manual mode.
    finally:
        server.server_close()                 # Release the listening socket.


if __name__ == '__main__':
    main()

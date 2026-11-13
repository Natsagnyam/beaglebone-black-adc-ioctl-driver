SUMMARY = "BBB frequency telemetry HTTP dashboard"
DESCRIPTION = "Read the TI AM335x IIO ADC, use the bbb_freq ioctl interface, and serve a browser gauge"
LICENSE = "GPL-2.0-only"
# Replace this placeholder with the actual checksum from your LICENSE file.
LIC_FILES_CHKSUM = "file://LICENSE;md5=REPLACE_WITH_REAL_MD5"

inherit systemd

SRC_URI = "file://dashboard.py \
           file://index.html \
           file://bbb-freq-dashboard.service \
           file://LICENSE"

S = "${WORKDIR}"

# Install files in standard Linux locations.
do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/dashboard.py ${D}${bindir}/bbb-freq-dashboard
    install -d ${D}${datadir}/bbb-freq
    install -m 0644 ${WORKDIR}/index.html ${D}${datadir}/bbb-freq/index.html
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bbb-freq-dashboard.service ${D}${systemd_system_unitdir}/
}

SYSTEMD_SERVICE:${PN} = "bbb-freq-dashboard.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

# Python modules needed for http.server, fcntl, json, struct, threading, pathlib.
RDEPENDS:${PN} += "python3-core python3-modules kernel-module-freq-driver"
FILES:${PN} += "${systemd_system_unitdir}/bbb-freq-dashboard.service ${datadir}/bbb-freq/index.html"

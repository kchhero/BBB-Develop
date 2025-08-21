FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
            file://10-eth0.network \
            "
# SRC_URI += " \
#             file://qt-simplebuttons.service \
#             "

PACKAGECONFIG:append = " networkd resolved"

do_install:append() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/10-eth0.network ${D}${sysconfdir}/systemd/network/10-eth0.network

    # install -d ${D}${systemd_unitdir}/system
    # install -m 0644 ${WORKDIR}/qt-simplebuttons.service ${D}${systemd_unitdir}/system/qt-simplebuttons.service
}

#SYSTEMD_SERVICE:${PN} += "qt-simplebuttons.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

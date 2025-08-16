FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
            file://eth0.network \
            file://qt-simplebuttons.service \
            "

do_install:append() {
    install -m 0644 ${WORKDIR}/eth0.network ${D}${sysconfdir}/systemd/network/

    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/qt-simplebuttons.service ${D}${systemd_unitdir}/system/qt-simplebuttons.service
}

#PACKAGECONFIG:remove = " binfmt"
#PACKAGES:remove = "${PN}-binfmt"
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://eth0.network"

do_install:append() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 /home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-core/systemd/files/eth0.network ${D}${sysconfdir}/systemd/network/
}

#PACKAGECONFIG:remove = " binfmt"
#PACKAGES:remove = "${PN}-binfmt"
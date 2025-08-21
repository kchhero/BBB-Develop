SUMMARY = "A web server to control BeagleBone Black's Servo motor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI = "file://bbb-servo-controller-0.1.0/ \
        "

S = "${WORKDIR}/bbb-servo-controller-0.1.0"

inherit cargo_bin

do_compile[network] = "1"

do_install:append() {
        #copy to build dir in local PC
        cp ${D}/usr/bin/bbb-servo-controller /home/suker/myYocto/poky/build-full-cmdline/

        install -d ${D}${datadir}/bbb-servo-controller
        #install -m 0644 ${WORKDIR}/index.html ${D}${datadir}/bbb-servo-controller/
}

#FILES:${PN} += "${datadir}/bbb-servo-controller/index.html"
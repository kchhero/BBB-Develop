SUMMARY = "Simple GPIO LED Toggle Driver"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit module

SRC_URI = "file://led_driver.c \
           file://Makefile \
          "

S = "${WORKDIR}"

do_install(){
        #install to /usr/bin/
        install -d ${D}${bindir}
        install -m 0755 ${S}/led_driver.ko ${D}${bindir}
}

# package skip
# [installed-vs-shipped]
INSANE_SKIP:${PN} += "installed-vs-shipped"
#FILES:${PN} += "${D}${bindir}/led_driver.ko"

#KERNEL_MODULE_AUTOLOAD = "led_driver"

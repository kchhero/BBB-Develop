FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://extlinux.conf \
    "

UBOOT_MMC_BOOT_BUILD_CONFIG ?= "0"


do_deploy:append() {
    bbnote ">>> Deploying custom extlinux.conf"
    install -Dm0644 /home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-bsp/u-boot/files/extlinux.conf ${DEPLOYDIR}/extlinux.conf
    install -Dm0644 /home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-bsp/u-boot/files/extlinux.conf ${SYSROOT_DESTDIR}/boot/extlinux/extlinux.conf
}

FILES:${PN} += "${sysconfdir}/boot/extlinux/extlinux.conf"

addtask deploy after do_compile before do_build
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://extlinux.conf \
    "

# `UBOOT_CUSTOM_CONFIG`라는 변수를 정의합니다.
# UBOOT_MMC_BOOT_BUILD_CONFIG 1 ==> finduuid=part uuid mmc 1:2 uuid\0
# UBOOT_MMC_BOOT_BUILD_CONFIG 0 ==> finduuid=part uuid mmc 0:2 uuid\0
UBOOT_MMC_BOOT_BUILD_CONFIG ?= "1"

# UBOOT_CUSTOM_CONFIG가 "1"일 경우에만 my-am335x-config.patch를 SRC_URI에 추가.
SRC_URI:append = "${@' file://mmc_boot_dev_change_to_1.patch' if d.getVar('UBOOT_MMC_BOO    T_BUILD_CONFIG') == '1' else ''}"

do_deploy:append() {
    bbnote ">>> Deploying custom extlinux.conf"
    install -Dm0644 /home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-bsp/u-boot/files/extlinux.conf ${DEPLOYDIR}/extlinux.conf
    install -Dm0644 /home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-bsp/u-boot/files/extlinux.conf ${SYSROOT_DESTDIR}/boot/extlinux/extlinux.conf
}

FILES:${PN} += "${sysconfdir}/boot/extlinux/extlinux.conf"

addtask deploy after do_compile before do_build

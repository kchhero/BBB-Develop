SUMMARY = "My custom GUI image for BeagleBone Black"
DESCRIPTION = "A customized image with Qt5 and touchscreen support for BeagleBone Black."
LICENSE = "MIT"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

#require recipes-core/images/core-image-minimal.bb
require recipes-extended/images/core-image-full-cmdline.bb
#require recipes-sato/images/core-image-sato.bb

ROOTFS_POSTPROCESS_COMMAND += " install_emmc_flash_script;"

SRC_URI += " \
    file://beaglebone-black-eMMC-flasher.sh \
    file://AI_version-beaglebone-black-eMMC-flasher.sh \
    "

install_emmc_flash_script() {
    install -d ${IMAGE_ROOTFS}/opt/scripts
    install -m 0755 ${THISDIR}/files/beaglebone-black-eMMC-flasher.sh ${IMAGE_ROOTFS}/opt/scripts/beaglebone-black-eMMC-flasher.sh
    install -m 0755 ${THISDIR}/files/AI_version-beaglebone-black-eMMC-flasher.sh ${IMAGE_ROOTFS}/opt/scripts/AI_version-beaglebone-black-eMMC-flasher.sh
}

# 이미지에 Qt5 및 tslib 구성 요소를 추가합니다.
IMAGE_INSTALL:append = " \
        qtbase \
        qtbase-plugins \
        tslib \        
        qtdeclarative \
        qtquickcontrols2 \        
        qt5ledscreen \
        qt5everywheredemo \
        parted \
        dosfstools \
        rsync \
        simplebuttons \
        i2c-tools \
        evtest \
        udev \
        "

PN = "suker-gui-image"

IMAGE_ROOTFS_SIZE = "1048576"
#IMAGE_ROOTFS_EXTRA_SPACE = "1048576"
IMAGE_OVERHEAD_FACTOR = "1.0"

SUMMARY = "My custom GUI image for BeagleBone Black"
DESCRIPTION = "A customized image with Qt5 and touchscreen support for BeagleBone Black."
LICENSE = "MIT"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

#require recipes-core/images/core-image-minimal.bb
#require recipes-extended/images/core-image-full-cmdline.bb
#require recipes-sato/images/core-image-sato.bb
require recipes-graphics/images/core-image-weston.bb

ROOTFS_POSTPROCESS_COMMAND += " install_emmc_flash_script;"

SRC_URI += " \
    file://AI_version-beaglebone-black-eMMC-flasher.sh \
    "

install_emmc_flash_script() {
    install -d ${IMAGE_ROOTFS}/opt/scripts
    install -m 0755 ${THISDIR}/files/AI_version-beaglebone-black-eMMC-flasher.sh ${IMAGE_ROOTFS}/opt/scripts/AI_version-beaglebone-black-eMMC-flasher.sh
}

# Utility
IMAGE_INSTALL:append = " \
        parted \
        dosfstools \
        rsync \
        i2c-tools \
        evtest \
        udev \
        minicom \
        usbutils \
        v4l-utils \
        "

# for camera
IMAGE_INSTALL:append = " \
        ffmpeg \
        libjpeg-turbo \
        libpng \
        libgpiod \
        gstreamer1.0 \
        alsa-utils \
        "

# for Qt5 and OpenCV
IMAGE_INSTALL:append = " \
        tslib \
        qtbase qtwayland qtdeclarative qtmultimedia gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
        bluez5 pulseaudio qtconnectivity \
        qtquickcontrols2 qtgraphicaleffects qtsvg qtquickcontrols \
        weston weston-examples weston-init \
        "

# for my applications
IMAGE_INSTALL:append = " \
        myivi \
        "

# IMAGE_INSTALL:remove = "mesa-pvr"
# MACHINE_FEATURES:remove = "gpu"

PN = "suker-ivi-image"

#IMAGE_ROOTFS_SIZE = "1048576"
IMAGE_ROOTFS_EXTRA_SPACE = "128"
#IMAGE_OVERHEAD_FACTOR = "1.0"

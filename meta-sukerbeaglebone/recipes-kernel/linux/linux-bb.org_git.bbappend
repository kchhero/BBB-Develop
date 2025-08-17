
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-bb.org:"

SRC_URI:remove = " \
    git://github.com/beagleboard/linux.git;protocol=https;branch=${BRANCH} \
"
# SRC_URI:append = " \
#     git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11;depth=5 \
# "

SRCREV:armv7a = "0b6042ce56a66944bf4abd404e2ad159a76d8fca"
PV:armv7a = "v6.12.23-ti-arm32-r11+git${SRCPV}"
BRANCH:armv7a = "v6.12.23-ti-arm32-r11"

SRC_URI := " \
    git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11 \
    file://defconfig \
    "
#file:///home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-kernel/linux/linux-bb.org/BB-MY-GPIO-TEST-00A0.dtso \
#"

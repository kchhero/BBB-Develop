
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-bb.org:"

SRC_URI:remove = " \
    git://github.com/beagleboard/linux.git;protocol=https;branch=${BRANCH} \
"
# SRC_URI:append = " \
#     git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11;depth=5 \
# "

SRCREV:armv7a = "0f1f4b11a1c3a61ce9420b44f2ab89233d636257"
PV:armv7a = "v6.12.23-ti-arm32-r11+git${SRCPV}"
BRANCH:armv7a = "v6.12.23-ti-arm32-r11"

SRC_URI := " \
    git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11 \
    file://defconfig \
    "
#file:///home/suker/myYocto/poky/meta-sukerbeaglebone/recipes-kernel/linux/linux-bb.org/BB-MY-GPIO-TEST-00A0.dtso \
#"

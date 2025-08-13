FILESEXTRAPATHS:prepend := "${THISDIR}/linux-bb.org:"

SRC_URI:remove = " \
    git://github.com/beagleboard/linux.git;protocol=https;branch=${BRANCH} \
"
# SRC_URI:append = " \
#     git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11;depth=5 \
# "

SRCREV:armv7a = "9fcc10549a76cab2f7b19a77c9720956aafe1271"
PV:armv7a = "6.1.80+git${SRCPV}"
BRANCH:armv7a = "v6.1.80-ti-r34"

SRC_URI := " \
    git:///home/suker/myYocto/MYSRC/linux-PC-v6.1.80-ti-r34;protocol=file;branch=v6.1.80-ti-r34 \
    file://defconfig \
    "

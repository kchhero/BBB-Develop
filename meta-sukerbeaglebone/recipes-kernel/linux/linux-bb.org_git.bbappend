
SRC_URI:remove = " \
    git://github.com/beagleboard/linux.git;protocol=https;branch=${BRANCH} \
"
# SRC_URI:append = " \
#     git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11;depth=5 \
# "

SRCREV:armv7a = "4ca9ea30768d58c8d4d56d03dd1eaf8c8feb7ef9"
PV:armv7a = "6.1.80+git${SRCPV}"
BRANCH:armv7a = "v6.1.80-ti-r34"

SRC_URI := " \
    git:///home/suker/myYocto/MYSRC/linux-PC-v6.1.80-ti-r34;protocol=file;branch=v6.1.80-ti-r34 \
    "
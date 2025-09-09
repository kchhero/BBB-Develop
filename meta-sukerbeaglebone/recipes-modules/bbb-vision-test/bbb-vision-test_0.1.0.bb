SUMMARY = "A vision tester to control BeagleBone Black's Servo motor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI = "file://main.cpp \
        "
S = "${WORKDIR}"

DEPENDS = " opencv i2c-tools"
RDEPENDS:${PN} += " i2c-tools"

inherit pkgconfig

do_compile() {
        ${CXX} ${CXXFLAGS} ${LDFLAGS} \
        -I ${STAGING_INCDIR} \
        $(pkg-config --cflags opencv4) \
        -o bbb-vision-test main.cpp \
        $(pkg-config --libs opencv4)
}

do_install:append() {
        install -d ${D}${bindir}
        install -m 0755 bbb-vision-test ${D}${bindir}

        #copy to build dir in local PC
        cp bbb-vision-test /home/suker/myYocto/poky/build-full-cmdline/
}

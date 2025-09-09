SUMMARY = "A vision run to control BeagleBone Black's Servo motor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI = " \
        file://main.cpp \
        file://robotArm.cpp \
        file://robotArm.h \
        file://pca9685.cpp \
        file://pca9685.h \
        "

S = "${WORKDIR}"

DEPENDS = " opencv i2c-tools"
inherit pkgconfig

# do_compile() {
#         ${CXX} ${CXXFLAGS} ${LDFLAGS} \
#         $(pkg-config --cflags opencv4) \
#         -o bbb-vision-run main.cpp \
#         $(pkg-config --libs opencv4)
# }

do_compile() {
        ${CXX} main.cpp pca9685.cpp robotArm.cpp -o bbb-vision-run \
        ${CXXFLAGS} ${LDFLAGS} \
        $(pkg-config --cflags --libs opencv4) \
        -lopencv_videoio \
        -L${STAGING_LIBDIR} -li2c
}

do_install:append() {
        install -d ${D}${bindir}
        install -m 0755 bbb-vision-run ${D}${bindir}

        #copy to build dir in local PC
        cp bbb-vision-run /home/suker/myYocto/poky/build-full-cmdline/
}

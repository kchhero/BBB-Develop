SUMMARY = "Simple Qt Buttons Example"
LICENSE = "MIT"
#LIC_FILES_CHKSUM = "file://README.md;md5=6dc51ef08169aeb1d70ab5867f57c97f"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://simplebuttons.pro \
           file://main.cpp \
"

S = "${WORKDIR}"

inherit qmake5

DEPENDS += "qtbase"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/simplebuttons ${D}${bindir}/simplebuttons
}

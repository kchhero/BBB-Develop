SUMMARY = "Minimal Qt IVI UI"
LICENSE = "MIT"
#LIC_FILES_CHKSUM = "file://README.md;md5=6dc51ef08169aeb1d70ab5867f57c97f"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://main.qml \
           file://myivi-ui.pro \
           file://main.cpp \
           file://qml.qrc \
           "

S = "${WORKDIR}"

inherit qmake5

DEPENDS += " \
        qtbase \
        qtsvg \
        qtdeclarative \
        qtmultimedia \
        qtconnectivity \
        qtquickcontrols \
        qtquickcontrols2 \
        qtgraphicaleffects \
        "

RDEPENDS:${PN} += " \
        qtdeclarative-qmlplugins \
        qtquickcontrols2-qmlplugins \
        qtmultimedia-qmlplugins \
        "

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/myivi-ui ${D}${bindir}/myivi-ui
}

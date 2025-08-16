FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://uEnv.txt"

do_install:append() {
    install -d ${D}/boot
    install -m 0644 ${THISDIR}/files/uEnv.txt ${D}/boot/uEnv.txt      
}

FILES:${PN} += "/boot/uEnv.txt"

DEVICE_TREE = "am335x-boneblack"
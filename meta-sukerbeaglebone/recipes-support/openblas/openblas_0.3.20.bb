SUMMARY = "An optimized BLAS library based on GotoBLAS2"
DESCRIPTION = "OpenBLAS is an optimized BLAS library based on GotoBLAS2 1.13 BSD version."
HOMEPAGE = "http://www.openblas.net"
LICENSE = "BSD-3-Clause"

SRC_URI = "https://github.com/xianyi/OpenBLAS/archive/v${PV}.tar.gz"
SRC_URI[sha256sum] = "8495c9affc536253648e942908e88e097f2ec7753ede55aca52e5dead3029e3c"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5adf4792c949a00013ce25d476a2abc0"

S = "${WORKDIR}/OpenBLAS-${PV}"

# BeagleBone Black (ARMv7) 타겟을 직접 명시하고 Fortran 빌드를 비활성화합니다.
EXTRA_OEMAKE = "NO_FORTRAN=1 HOSTCC='${BUILD_CC}' CC='${CC}' FC='' TARGET=ARMV7"
EXTRA_OECMAKE += " -DBUILD_FORTRAN=OFF"

PARALLEL_MAKE = ""

do_install() {
    install -d ${D}${libdir}
    install -d ${D}${includedir}

    # Use 'install' for the library to set correct ownership (root:root)
    # This copies the real library file first
    install -m 0755 ${B}/libopenblas_armv7p-r0.3.20.so ${D}${libdir}/libopenblas.so.0

    # Now, create the symbolic links manually inside the correct directory
    ln -sf libopenblas.so.0 ${D}${libdir}/libopenblas.so

    # Use 'install' for headers to set correct ownership
    install -m 0644 ${B}/cblas.h ${D}${includedir}
    install -m 0644 ${B}/lapack-netlib/LAPACKE/include/lapacke.h ${D}${includedir}
}

# FILES:${PN} = ""
# FILES:${PN}-dev = ""

# # 런타임용(.so.*)과 개발용(.so) 라이브러리를 명확히 분리합니다.
# FILES:${PN} += "${libdir}/libopenblas.so.*"
# FILES:${PN}-dev += "${libdir}/libopenblas.so ${libdir}/*.a ${includedir} ${libdir}/cmake"

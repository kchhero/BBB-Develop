# meta-sukerbeaglebone/recipes-support/opencv/opencv_%.bbappend

# allow appending local files
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# 1) 최소 PACKAGECONFIG: core + image codecs (jpeg/png/tiff) 등만 허용
#    (여기에 필요없는 옵션들을 모두 제거)
PACKAGECONFIG:remove = " gstreamer gtk gl opengl"
PACKAGECONFIG:append = " jpeg png tiff contrib"

# 2) (선택) opencv_contrib 사용 예시: 소스 추가 및 cmake 옵션
#    주석 처리 해제하면 contrib을 함께 빌드합니다 (원하면 활성화)
SRC_URI:append = " \    
    git://github.com/opencv/opencv_contrib.git;destsuffix=opencv_contrib;name=contrib;branch=master;protocol=https \
"

CXXFLAGS:append = " -latomic"

EXTRA_OECMAKE:append = " -DOPENCV_EXTRA_MODULES_PATH=${WORKDIR}/opencv_contrib/modules "

# 3) 확실하게 GUI / GStreamer 등 끄기 (CMake 플래그)
EXTRA_OECMAKE:append = " -DBUILD_opencv_highgui=OFF -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_OPENGL=OFF -DWITH_VTK=OFF"

EXTRA_OECMAKE:append = " \
    -DWARNINGS_ARE_ERRORS=OFF \    
    -DCMAKE_C_FLAGS='${CFLAGS} -mfpu=neon -mfloat-abi=hard' \
    -DCMAKE_CXX_FLAGS='${CXXFLAGS} -mfpu=neon -mfloat-abi=hard' \
    -DENABLE_NEON=ON \
    -DENABLE_VFPV3=ON \
    -DENABLE_FP16=OFF \
    -DWITH_V4L=ON \
    -DWITH_LIBV4L=ON \
    -DWITH_V4L2=ON \
    -DHAVE_V4L1=OFF \
    -DHAVE_ALIGNED_MALLOC=OFF \
    -DPYTHON2_EXECUTABLE=OFF \
    -DBLAS_LIBRARIES=${STAGING_LIBDIR}/libopenblas.so -DLAPACK_LIBRARIES=${STAGING_LIBDIR}/libopenblas.so \
    -DWITH_OPENBLAS=ON \    
"

DEPENDS:append = " openblas libpng"

PSEUDO_IGNORE_PATHS .= ",${WORKDIR}/opencv_contrib"
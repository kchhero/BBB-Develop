# BBB-Rust-Driver

I'm experimenting and developing various things on the BeagleBone Black board.
I plan to create a binary using Yocto Base.
I'm using the Kirkstone version of Yocto.
My PC is Ubuntu 16.04.
I'm connecting a Waveshare 7" HDMI-LCD-(C) and building a simple user input environment using a QT app. For example, pressing touch button A will activate GPIO pin 1 high. The specific operation method will be documented on this page.
I created the repository on August 12, 2025, but I've already completed the Yocto build configuration and LCD bring-up. There was a slight error because the touch operation was handled by the USB-HID driver, not I2C.

I plan to provide daily progress updates starting August 13, 2025.

## bblayer.conf
BBLAYERS ?= " \
  /home/suker/myYocto/poky/meta \
  /home/suker/myYocto/poky/meta-poky \
  /home/suker/myYocto/poky/meta-yocto-bsp \
  /home/suker/myYocto/poky/meta-openembedded/meta-oe \
  /home/suker/myYocto/poky/meta-openembedded/meta-multimedia \
  /home/suker/myYocto/poky/meta-openembedded/meta-python \
  /home/suker/myYocto/poky/meta-arm/meta-arm-toolchain \
  /home/suker/myYocto/poky/meta-arm/meta-arm \
  /home/suker/myYocto/poky/meta-ti/meta-ti-bsp \
  /home/suker/myYocto/poky/meta-qt5 \
  /home/suker/myYocto/poky/meta-sukerbeaglebone \
  "
  
## local.conf
MACHINE ?= "beaglebone"

DISTRO ?= "poky"
PACKAGE_CLASSES ?= "package_rpm"

EXTRA_IMAGE_FEATURES ?= "debug-tweaks"

USER_CLASSES ?= "buildstats"

PATCHRESOLVE = "noop"

BB_DISKMON_DIRS ??= "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    STOPTASKS,/tmp,100M,100K \
    HALT,${TMPDIR},100M,1K \
    HALT,${DL_DIR},100M,1K \
    HALT,${SSTATE_DIR},100M,1K \
    HALT,/tmp,10M,1K"

PACKAGECONFIG:append:pn-qemu-system-native = " sdl"

CONF_VERSION = "2"

MYDIR := "/home/suker/myYocto"
DL_DIR ?= "${MYDIR}/downloads"
SSTATE_DIR ?= "${MYDIR}/sstate-cache"
TMPDIR = "${TOPDIR}/tmp"
RM_OLD_IMAGE = "1"

DISTRO_FEATURES = "systemd udev opengl alsa splash"
DISTRO_FEATURES:remove = " wayland"

VIRTUAL-RUNTIME_init_manager = "systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"

PACKAGECONFIG:remove:pn-qtbase = "wayland"
#PACKAGECONFIG:append:pn-qtbase = "x11"
PACKAGECONFIG:append:pn-qtbase = " eglfs linuxfb"

LICENSE_FLAGS_ACCEPTED = "ti-sgx-ddk-um"

UBOOT_MMC_BOOT_BUILD_CONFIG = "1"

BB_NUMBER_THREADS ?= "8"
PARALLEL_MAKE ?= "-j 8"


## linux-kernel
### source download
  $ git clone https://git.ti.com/git/ti-linux-kernel/ti-linux-kernel.git linux-bbb-local
  
  $ cd linux-bbb-local
  
  $ git checkout c99a15d2677b251e5de6c5f28eafc5042e02dc6e
  
  
  It is the same branch as meta-ti kirkstone, SRCREV version.
  For convenience, the source code is downloaded separately to modify the source code and develop the device driver directly on the local PC.

## u-boot
  I decided to use u-boot as is, but I overrode only the dtb file.
  See, u-boot-ti-staging_%.bbappend  ==> am335x-boneblack-revd.dtb
  


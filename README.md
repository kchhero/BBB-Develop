# BBB-Rust-Driver

I'm experimenting and developing various things on the BeagleBone Black board.
I plan to create a binary using Yocto Base.
I'm using the Kirkstone version of Yocto.
My PC is Ubuntu 16.04.
I'm connecting a Waveshare 7" HDMI-LCD-(C) and building a simple user input environment using a QT app. For example, pressing touch button A will activate GPIO pin 1 high. The specific operation method will be documented on this page.
I created the repository on August 12, 2025, but I've already completed the Yocto build configuration and LCD bring-up. There was a slight error because the touch operation was handled by the USB-HID driver, not I2C.

I plan to provide daily progress updates starting August 13, 2025.

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
  


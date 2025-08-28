## ESP32 develop package install
### ubuntu package
> $ sudo apt update
>  $ sudo apt install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util

### ESP-IDF download
>  $ mkdir -p ~/esp

>  $ cd ~/esp

>  $ git clone --recursive https://github.com/espressif/esp-idf.git

>  $ cd esp-idf

### environmemt setup
>  $ ./install.sh

>  $ . ./export.sh

### firmware flashing
>  $ idf.py -p /dev/ttyUSB0 flash        #Firmware Upload

>  $ idf.py -p /dev/ttyUSB0 monitor      #Serial Log

=============================================================

## ESP32 Section
#### Bootloader
  - bootloader.bin
#### Partition Table : Firmware, NVS, OTA, etc...
  - partition-table.bin
#### Application Firmware	: Real active user application (hello world, sensor, communication ...)
  - your_app.bin (*.elf나 *.bin)

=============================================================

## How to build firmware
#### step1 : ESP-IDF env activate
  $ . ~/esp/esp-idf/export.sh
#### step2 : new project create
>  $ cd ~/esp

>  $ idf.py create-project my_app

>  $ cd my_app
#### step3 : example
>  main/main.c -> hello world
#### step4 : build
>  $ idf.py build
#### step5 : output .bin 
#### step6 : flashing
>  $ idf.py -p /dev/ttyUSB0 flash

>  $ idf.py -p /dev/ttyUSB0 monitor

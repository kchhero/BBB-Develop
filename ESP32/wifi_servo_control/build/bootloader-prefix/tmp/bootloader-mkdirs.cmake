# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/suker/tmp/ESP32/esp-idf/components/bootloader/subproject"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/tmp"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/src/bootloader-stamp"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/src"
  "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/suker/tmp/ESP32/esp-idf/wifi_servo_control/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

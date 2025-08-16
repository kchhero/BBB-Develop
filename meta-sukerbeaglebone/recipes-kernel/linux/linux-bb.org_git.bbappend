
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-bb.org:"

SRC_URI:remove = " \
    git://github.com/beagleboard/linux.git;protocol=https;branch=${BRANCH} \
"
# SRC_URI:append = " \
#     git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11;depth=5 \
# "

SRCREV:armv7a = "fdfa770b41b1429021d70dbd04c0b00a8fc2ed81"
PV:armv7a = "v6.12.23-ti-arm32-r11+git${SRCPV}"
BRANCH:armv7a = "v6.12.23-ti-arm32-r11"

SRC_URI := " \
    git:///home/suker/myYocto/MYSRC/linux;protocol=file;branch=v6.12.23-ti-arm32-r11 \
    file://defconfig \
    "

# do_deploy:append() {
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblack-revd.dtb                    ${DEPLOYDIR}/am335x-boneblack-revd.dtb    
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-bone.dtb                              ${DEPLOYDIR}/am335x-bone.dtb                         
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblack.dtb                         ${DEPLOYDIR}/am335x-boneblack.dtb                    
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblack-uboot.dtb                   ${DEPLOYDIR}/am335x-boneblack-uboot.dtb              
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblack-uboot-univ.dtb              ${DEPLOYDIR}/am335x-boneblack-uboot-univ.dtb         
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblack-wireless.dtb                ${DEPLOYDIR}/am335x-boneblack-wireless.dtb           
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-boneblue.dtb                          ${DEPLOYDIR}/am335x-boneblue.dtb                     
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-bonegreen.dtb                         ${DEPLOYDIR}/am335x-bonegreen.dtb                    
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-bonegreen-gateway.dtb                 ${DEPLOYDIR}/am335x-bonegreen-gateway.dtb            
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-bonegreen-wireless.dtb                ${DEPLOYDIR}/am335x-bonegreen-wireless.dtb           
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/am335x-bonegreen-wireless-uboot-univ.dtb     ${DEPLOYDIR}/am335x-bonegreen-wireless-uboot-univ.dtb

#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/AM335X-PRU-UIO-00A0.dtbo             ${DEPLOYDIR}/AM335X-PRU-UIO-00A0.dtbo  
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-ADC-00A0.dtbo                     ${DEPLOYDIR}/BB-ADC-00A0.dtbo          
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BBBW-WL1835-00A0.dtbo             ${DEPLOYDIR}/BB-BBBW-WL1835-00A0.dtbo  
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BBGG-WL1835-00A0.dtbo             ${DEPLOYDIR}/BB-BBGG-WL1835-00A0.dtbo  
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BBGW-WL1835-00A0.dtbo             ${DEPLOYDIR}/BB-BBGW-WL1835-00A0.dtbo  
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BONE-4D5R-01-00A1.dtbo            ${DEPLOYDIR}/BB-BONE-4D5R-01-00A1.dtbo 
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BONE-eMMC1-01-00A0.dtbo           ${DEPLOYDIR}/BB-BONE-eMMC1-01-00A0.dtbo
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BONE-LCD4-01-00A1.dtbo            ${DEPLOYDIR}/BB-BONE-LCD4-01-00A1.dtbo 
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-BONE-NH7C-01-A0.dtbo              ${DEPLOYDIR}/BB-BONE-NH7C-01-A0.dtbo   
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-CAPE-DISP-CT4-00A0.dtbo           ${DEPLOYDIR}/BB-CAPE-DISP-CT4-00A0.dtbo
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-HDMI-TDA998x-00A0.dtbo            ${DEPLOYDIR}/BB-HDMI-TDA998x-00A0.dtbo 
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-I2C1-MCP7940X-00A0.dtbo           ${DEPLOYDIR}/BB-I2C1-MCP7940X-00A0.dtbo
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-I2C1-RTC-DS3231.dtbo              ${DEPLOYDIR}/BB-I2C1-RTC-DS3231.dtbo   
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-I2C1-RTC-PCF8563.dtbo             ${DEPLOYDIR}/BB-I2C1-RTC-PCF8563.dtbo  
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-I2C2-BME680.dtbo                  ${DEPLOYDIR}/BB-I2C2-BME680.dtbo       
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-I2C2-MPU6050.dtbo                 ${DEPLOYDIR}/BB-I2C2-MPU6050.dtbo          
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-NHDMI-TDA998x-00A0.dtbo           ${DEPLOYDIR}/BB-NHDMI-TDA998x-00A0.dtbo
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BBORG_COMMS-00A2.dtbo                ${DEPLOYDIR}/BBORG_COMMS-00A2.dtbo     
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BBORG_FAN-A000.dtbo                  ${DEPLOYDIR}/BBORG_FAN-A000.dtbo       
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BBORG_RELAY-00A2.dtbo                ${DEPLOYDIR}/BBORG_RELAY-00A2.dtbo     
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-SPIDEV0-00A0.dtbo                 ${DEPLOYDIR}/BB-SPIDEV0-00A0.dtbo      
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-SPIDEV1-00A0.dtbo                 ${DEPLOYDIR}/BB-SPIDEV1-00A0.dtbo      
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-UART1-00A0.dtbo                   ${DEPLOYDIR}/BB-UART1-00A0.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-UART2-00A0.dtbo                   ${DEPLOYDIR}/BB-UART2-00A0.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-UART4-00A0.dtbo                   ${DEPLOYDIR}/BB-UART4-00A0.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-W1-P9.12-00A0.dtbo                ${DEPLOYDIR}/BB-W1-P9.12-00A0.dtbo     
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BONE-ADC.dtbo                        ${DEPLOYDIR}/BONE-ADC.dtbo             
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/M-BB-BBG-00A0.dtbo                   ${DEPLOYDIR}/M-BB-BBG-00A0.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/M-BB-BBGG-00A0.dtbo                  ${DEPLOYDIR}/M-BB-BBGG-00A0.dtbo       
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/PB-MIKROBUS-0.dtbo                   ${DEPLOYDIR}/PB-MIKROBUS-0.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/PB-MIKROBUS-1.dtbo                   ${DEPLOYDIR}/PB-MIKROBUS-1.dtbo        
#     install -Dm0644 ${B}/arch/arm/boot/dts/ti/omap/BB-LCD-ADAFRUIT-24-SPI1-00A0.dtbo    ${DEPLOYDIR}/BB-LCD-ADAFRUIT-24-SPI1-00A0.dtbo
# }

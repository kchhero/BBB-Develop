#!/bin/sh
# BeagleBone Black HDMI reset script
# GPIO1_25 → sysfs 번호 57

GPIO_SYSFS_NUM=57
GPIOCHIP_BANK=1
GPIOCHIP_LINE=25

echo "[INFO] HDMI Reset Script 시작"

# 1. sysfs 방식 시도
if [ -d /sys/class/gpio ]; then
    echo "[INFO] sysfs 방식으로 GPIO 설정"
    if [ ! -d /sys/class/gpio/gpio$GPIO_SYSFS_NUM ]; then
        echo $GPIO_SYSFS_NUM > /sys/class/gpio/export
    fi
    echo out > /sys/class/gpio/gpio$GPIO_SYSFS_NUM/direction
    echo 1 > /sys/class/gpio/gpio$GPIO_SYSFS_NUM/value
else
    echo "[WARN] sysfs GPIO 없음 → libgpiod 방식 시도"
    if command -v gpioset >/dev/null 2>&1; then
        gpioset gpiochip$GPIOCHIP_BANK $GPIOCHIP_LINE=1
    else
        echo "[ERROR] libgpiod(gpioset)도 설치되어 있지 않음"
        exit 1
    fi
fi

# 2. HDMI 칩 I²C 주소 확인
echo "[INFO] HDMI 칩 I2C 주소 확인"
if command -v i2cdetect >/dev/null 2>&1; then
    i2cdetect -y 0 | grep "70"
else
    echo "[WARN] i2cdetect 없음"
fi

# 3. tda998x 드라이버 재로드
echo "[INFO] tda998x 드라이버 재로드"
if lsmod | grep -q tda998x; then
    rmmod tda998x
fi
modprobe tda998x

echo "[INFO] 완료"

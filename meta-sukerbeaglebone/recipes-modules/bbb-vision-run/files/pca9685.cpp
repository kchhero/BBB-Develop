#include "pca9685.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <cstdio>

using namespace std;

inline int i2c_smbus_write_byte_data(int fd, uint8_t command, uint8_t value) {
    union i2c_smbus_data data;
    data.byte = value;
    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_WRITE;
    args.command = command;
    args.size = I2C_SMBUS_BYTE_DATA;
    args.data = &data;
    return ioctl(fd, I2C_SMBUS, &args);
}

static inline int i2c_smbus_write_i2c_block_data(int file, __u8 command, __u8 length, const __u8 *values)
{
    union i2c_smbus_data data;
    int i;

    if (length > 32)
        length = 32;
    data.block[0] = length;
    for (i = 1; i <= length; i++)
        data.block[i] = values[i-1];

    struct i2c_smbus_ioctl_data args = {
        .read_write = I2C_SMBUS_WRITE,
        .command = command,
        .size = I2C_SMBUS_I2C_BLOCK_DATA,
        .data = &data
    };

    return ioctl(file, I2C_SMBUS, &args);
}

PCA9685::PCA9685(const char* dev, int address) : fd(-1), is_ok(false) {
        if ((fd = open(dev, O_RDWR)) < 0) {
                perror("Failed to open I2C device");
                return;
        }
        if (ioctl(fd, I2C_SLAVE, address) < 0) {
                perror("Failed to set I2C_SLAVE address");
                close(fd);
                return;
        }
        is_ok = true;
}

PCA9685::~PCA9685() {
        if (fd >= 0) close(fd);
}

bool PCA9685::isOk() const {
        return is_ok;
}

void PCA9685::setFreq(int freq) {
        int prescale = static_cast<int>((25000000.0 / (4096.0 * freq)) - 1.0);
        i2c_smbus_write_byte_data(fd, 0x00, 0x10); // Sleep
        i2c_smbus_write_byte_data(fd, 0xFE, prescale);
        i2c_smbus_write_byte_data(fd, 0x00, 0x00); // Wake
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        i2c_smbus_write_byte_data(fd, 0x00, 0xA1); // Restart + Auto-increment
}

void PCA9685::setPWM(int channel, int on, int off) {
        __u8 reg = 0x06 + 4 * channel;
        __u8 data[4] = {
            static_cast<__u8>(on & 0xFF),
            static_cast<__u8>(on >> 8),
            static_cast<__u8>(off & 0xFF),
            static_cast<__u8>(off >> 8)
        };

        if (i2c_smbus_write_i2c_block_data(fd, reg, 4, data) < 0) {
            perror("Failed to write PWM data");
        }
}

int PCA9685::angle_to_pulse(double angle) const {
    // 0~180도를 0~4095 펄스로 변환
    double pulse_min = 150.0;  // 0 deg
    double pulse_max = 600.0;  // 180 deg

    if (angle < 0.0) {
        angle = 0.0;
    } else if (angle > 180.0) {
        angle = 180.0;
    }

    return static_cast<int>(pulse_min + (pulse_max - pulse_min) * (angle / 180.0));
}

#ifndef PCA9685_H
#define PCA9685_H

#include <cstdint>

class PCA9685 {
public:
    PCA9685(const char* device, int address);
    ~PCA9685();
    bool isOk() const;
    void setFreq(int freq);
    void setPWM(int channel, int on, int off);
    int angle_to_pulse(double angle) const;
private:
    int fd;
    bool is_ok;
};

#endif // PCA9685_H

// Robot Arm Servo Configuration Tool (Refactored)
// PCA9685 I2C control: ioctl + i2c_smbus_write_i2c_block_data 
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits> 
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPEED_FACTOR 0.2 //Maximum speed factor is 0.2 ~ 0.3

#define __suker__debug__ 1

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

// --- 서보 설정 구조체 ---
struct ServoConfig {
    int ch;
    std::string name;
    double standby_angle;
    double min_angle;
    double max_angle;
};

const char* I2C_BUS = "2";
const char* PCA_ADDRESS = "0x40";

class PCA9685 {
private:
    int fd;
    int addr;

public:
    PCA9685(const char* dev, int address) : fd(-1), addr(address) {
        if ((fd = open(dev, O_RDWR)) < 0) {
            perror("Failed to open I2C device");
            exit(1);
        }
        if (ioctl(fd, I2C_SLAVE, addr) < 0) {
            perror("Failed to set I2C_SLAVE address");
            close(fd);
            exit(1);
        }
        std::cout << "PCA9685 initialized (addr=0x" 
                  << std::hex << addr << std::dec << ")" << std::endl;
    }

    ~PCA9685() {
        if (fd >= 0) close(fd);
    }

    void setFreq(int freq) {
#ifdef __suker__debug__
        std::cout << "Setting PCA9685 frequency to " << freq << " Hz" << std::endl;
#endif
        int prescale = static_cast<int>((25000000.0 / (4096.0 * freq)) - 1.0);
        i2c_smbus_write_byte_data(fd, 0x00, 0x10); // Sleep
        i2c_smbus_write_byte_data(fd, 0xFE, prescale);
        i2c_smbus_write_byte_data(fd, 0x00, 0x00); // Wake
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        i2c_smbus_write_byte_data(fd, 0x00, 0xA1); // Restart + Auto-increment
#ifdef __suker__debug__
        std::cout << "PCA9685 frequency set complete." << std::endl;
#endif
    }

    void setPWM(int channel, int on, int off) {
#ifdef __suker__debug__
        std::cout << "Set PWM - Ch: " << channel << ", ON: " << on << ", OFF: " << off << std::endl;
#endif
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
};

int angle_to_pulse(double angle) {
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

// speed_dps: degree per second
void smooth_move(PCA9685& pca, std::map<int, double>& current_angles,
                 int channel, double target_angle, double speed_dps,
                 bool initial_move = false)
{
    double start_angle = current_angles.at(channel);
    double total_distance = target_angle - start_angle;
    if (!initial_move && std::abs(total_distance) < 0.1) return;

    // MG996R 최대 속도 기준
    // step당 1 deg로 나누고, 이동 시간 계산
    int total_steps = static_cast<int>(std::ceil(std::abs(total_distance)));
    if (total_steps < 1) {
        total_steps = 1;
    }

#ifdef __suker__debug__
    std::cout << "Moving Ch " << channel << " from " << start_angle 
              << " to " << target_angle << " (" << total_distance 
              << " deg) in " << total_steps << " steps at " 
              << speed_dps << " deg/s" << std::endl;
#endif
    double total_duration_s = std::abs(total_distance) / speed_dps;
    auto delay_per_step = std::chrono::milliseconds(
        std::max(1L, static_cast<long>((total_duration_s * 1000.0) / total_steps))
    );

    for (int step = 1; step <= total_steps; ++step) {
        double progress = static_cast<double>(step) / total_steps;
        // ease-in-out
        double ease_factor = (1.0 - std::cos(progress * M_PI)) / 2.0;
        double current_angle = start_angle + total_distance * ease_factor;
        pca.setPWM(channel, 0, angle_to_pulse(current_angle));
        std::this_thread::sleep_for(delay_per_step);
    }

    current_angles[channel] = target_angle;
}

void move_to_target(PCA9685& pca,
                    std::map<int, double>& current_angles,
                    const ServoConfig& config,
                    double target_angle,
                    double speed_dps)
{
    if (target_angle < config.min_angle) target_angle = config.min_angle;
    if (target_angle > config.max_angle) target_angle = config.max_angle;

    std::cout << "[MOVE] " << config.name
              << " (Ch " << config.ch << ")"
              << " Current: " << current_angles[config.ch]
              << " → Target: " << target_angle
              << " (범위: " << config.min_angle << " ~ " << config.max_angle << ")"
              << std::endl;

    smooth_move(pca, current_angles, config.ch, target_angle, speed_dps);
}

template<typename T>
T get_user_input(const std::string& prompt) {
    T value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.good()) {
            break;
        } else {
            std::cerr << "오류: 잘못된 입력입니다. 다시 시도하세요." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    return value;
}

// *** 변경점: 중복 제거를 위한 테스트 시퀀스 함수 ***
void perform_test_sequence(PCA9685& pca, std::map<int, double>& states, const ServoConfig& config, double speed) {
    std::cout << config.name << " 테스트 시퀀스 시작..." << std::endl;
    std::cout << "1. Min 위치(" << config.min_angle << ")로 이동..." << std::endl;
    smooth_move(pca, states, config.ch, config.min_angle, speed);
    std::cout << "2. Max 위치(" << config.max_angle << ")로 이동..." << std::endl;
    smooth_move(pca, states, config.ch, config.max_angle, speed);
    std::cout << "3. Standby 위치(" << config.standby_angle << ")로 복귀..." << std::endl;
    smooth_move(pca, states, config.ch, config.standby_angle, speed);
    std::cout << "테스트 완료." << std::endl;
}

int main() {
    // --- 1. 초기 서보 설정 ---
    std::map<int, ServoConfig> motor_configs_zero = {
        {15, {15, "1. 베이스",   90.0, 60.0, 120.0}},
        {14, {14, "2. 어깨",     90.0, 60.0, 120.0}},
        //{13, {13, "3. 팔꿈치",   90.0, 60.0, 120.0}},
        {12, {12, "4. 손목상하", 120.0, 90.0, 150.0}},
        {7,  {7, "5. 손목좌우",  90.0, 60.0, 120.0}},
        // {6,  {6, "6. 집게",      90.0, 60.0, 120.0}}
    };

    std::map<int, ServoConfig> motor_configs = {
        {15, {15, "1. 베이스",   95.0,  30.0, 160.0}},
        {14, {14, "2. 어깨",     90.0,  20.0, 100.0}},
        //{13, {13, "3. 팔꿈치",   100.0, 40.0, 150.0}},
        {12, {12, "4. 손목상하",  130.0, 30.0, 170.0}},
        {7,  {7, "5. 손목좌우",   90.0, 0.0, 180.0}},
        // {6,  {6, "6. 집게",       90.0, 0.0, 180.0}}
    };

    std::map<int, double> motor_states;
    const double MAX_SERVO_SPEED_DPS = 540.0; // maximum speed normally (deg/sec)
#ifdef __suker__debug__
    double motor_speed = MAX_SERVO_SPEED_DPS * SPEED_FACTOR;
#else
    const double motor_speed = MAX_SERVO_SPEED_DPS * SPEED_FACTOR;
#endif

    // --- 2. PCA9685 초기화 ---
    PCA9685 pca("/dev/i2c-2", 0x40);
    pca.setFreq(50);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //std::cout << "시스템 시작... 모든 서보를 Standby 위치로 부드럽게 이동합니다." << std::endl;
    for (const auto& [ch, config] : motor_configs_zero) {
        pca.setPWM(config.ch, 0, angle_to_pulse(config.standby_angle));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (const auto& [ch, config] : motor_configs) {
        motor_states[ch] = config.standby_angle;
        //smooth_move(pca, motor_states, config.ch, config.standby_angle, motor_speed);
        pca.setPWM(config.ch, 0, angle_to_pulse(config.standby_angle));
    }
    std::cout << "초기 위치 설정 완료." << std::endl;

    // --- 3. 사용자 입력 루프 ---
    while (true) {
// #ifdef __suker__debug__
//         std::cout << "\n\nWant speed factor (0.1 ~ 1.0) [현재: " << SPEED_FACTOR << "]: ";
//         double input_speed;
//         std::cin >> input_speed;
//         if (input_speed >= 0.1 && input_speed <= 1.0)
//             motor_speed = MAX_SERVO_SPEED_DPS * input_speed;
//         else
//             std::cout << "Invalid speed factor. Keeping previous value: " << motor_speed << " deg/s" << std::endl;
// #endif

        std::cout << "\n--- 현재 서보 설정 (속도: " << motor_speed << " deg/s) ---" << std::endl;        
        for (const auto& [ch, config] : motor_configs) {
            std::cout << "채널 " << ch << " (" << config.name << "): standby=" << config.standby_angle
                      << ", min=" << config.min_angle << ", max=" << config.max_angle << std::endl;
        }
        std::cout << "----------------------" << std::endl;
        std::cout << "77. move to target angle test mode" << std::endl;
        std::cout << "88. test mode (move min, max, standby)" << std::endl;
        std::cout << "99. all reset and init value position move~" << std::endl;
        std::cout << "----------------------" << std::endl;

        int channel_num = get_user_input<int>("명령을 입력하세요: ");

        if (channel_num == 99) {
            std::cout << "\n>> 모든 서보를 Standby 위치로 리셋합니다..." << std::endl;
            for (const auto& [ch, config] : motor_configs) {
                std::cout << config.name << " 이동 중..." << std::endl;
                smooth_move(pca, motor_states, config.ch, config.standby_angle, motor_speed, false);
            }
            std::cout << ">> 리셋 완료." << std::endl;
            continue;
        }
        else if (channel_num == 88) {
            // min ~ max 까지 천천히 움직이기 테스트
            std::cout << "\n>> 서보 동작 테스트 모드 (채널 번호 입력)" << std::endl;
            std::cout << "test channel number: " << channel_num << std::endl;
            std::cin >> channel_num;
            if (motor_configs.find(channel_num) == motor_configs.end()) {
                std::cerr << "오류: 존재하지 않는 채널 번호입니다." << std::endl;
                continue;
            }
            else {
                const auto& config = motor_configs.at(channel_num);
                std::cout << config.name << " 테스트 중..." << std::endl;
                std::cout << "ch " << channel_num << " move to min " << config.min_angle << std::endl;
                smooth_move(pca, motor_states, channel_num, config.min_angle, motor_speed, false);
                std::cout << "ch " << channel_num << " move to max " << config.max_angle << std::endl;
                smooth_move(pca, motor_states, channel_num, config.max_angle, motor_speed, false);
                std::cout << "ch " << channel_num << " move to standby " << config.standby_angle << std::endl;
                smooth_move(pca, motor_states, channel_num, config.standby_angle, motor_speed, false);
                std::cout << "테스트 완료." << std::endl;
                continue;
            }
        }
        else if (channel_num == 77) {  // target angle test 모드
            int ch = get_user_input<int>("Input Channel Number: ");
            if (motor_configs.find(ch) == motor_configs.end()) {
                std::cerr << "Not existed channel" << std::endl;
                continue;
            }

            const auto& config = motor_configs.at(ch);
            double target;

            while (true) {
                std::string temp_prompt = "이동할 Target Angle (" +
                    std::to_string(static_cast<int>(config.min_angle)) + " ~ " +
                    std::to_string(static_cast<int>(config.max_angle)) + "): ";

                target = get_user_input<double>(temp_prompt);

                if (target >= config.min_angle && target <= config.max_angle) {
                    break; // 올바른 값이면 탈출
                } else {
                    std::cerr << "범위를 벗어난 값입니다. 다시 입력하세요." << std::endl;
                }
            }
            move_to_target(pca, motor_states, config, target, motor_speed);
        }

        if (channel_num == 0) {
            break;
        }
        if (motor_configs.find(channel_num) == motor_configs.end()) {
            std::cerr << "오류: 존재하지 않는 채널 번호입니다." << std::endl;
            continue;
        }

        double new_standby = get_user_input<double>("새로운 Standby 각도를 입력하세요 (0 ~ 180): ");
        double new_move_value = get_user_input<double>("Standby 기준 좌우 동작 범위(Move Value)를 입력하세요: ");
        

        // --- 4. 설정 업데이트 ---
        auto& config = motor_configs.at(channel_num);
        config.standby_angle = new_standby;
        config.min_angle = new_standby - new_move_value;
        config.max_angle = new_standby + new_move_value;

        std::cout << "\n>> 채널 " << channel_num << " 업데이트 완료: standby=" << config.standby_angle
                  << ", min=" << config.min_angle << ", max=" << config.max_angle << std::endl;
        std::cout << ">> move test with new configurations" << std::endl;
        
        std::cout << "\n>> 채널 " << channel_num << " 업데이트 완료." << std::endl;
        perform_test_sequence(pca, motor_states, config, motor_speed);
    }

    std::cout << "프로그램을 종료합니다." << std::endl;
    return 0;
}

// #include <opencv2/opencv.hpp>
// #include <iostream>
// using namespace cv;
// using namespace std;

// int main() {
//     // V4L2 백엔드 명시
//     VideoCapture cap(0, cv::CAP_V4L2);
//     if (!cap.isOpened()) {
//         cerr << "카메라 열기 실패!" << endl;
//         return -1;
//     }
//     cout << "✅ 카메라 열기 성공" << endl;

//     // MJPEG, 640x480, 30fps 강제 설정
//     cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
//     cap.set(CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(CAP_PROP_FRAME_HEIGHT, 480);
//     cap.set(CAP_PROP_FPS, 30);

//     Mat frame;
//     if (!cap.read(frame)) {
//         cerr << "첫 프레임 캡처 실패!" << endl;
//         return -1;
//     }

//     cout << "✅ 프레임 캡처 성공" << endl;
//     imwrite("capture.jpg", frame);
//     cout << "📷 저장 완료: capture.jpg" << endl;

//     return 0;
// }

// Robot Arm Servo Configuration Tool
// This program allows dynamic configuration of servo motor parameters
// and demonstrates the changes by moving the servos accordingly.
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <algorithm> // for std::clamp

// --- 서보 설정 구조체 ---
struct ServoConfig {
    int ch;
    std::string name;
    double standby_angle; // UI 기준 각도 (-90 ~ 90)
    double min_angle;     // UI 기준 최소 각도
    double max_angle;     // UI 기준 최대 각도
};

// --- i2cset 기반 PCA9685 클래스 (동작 확인 버전) ---
const char* I2C_BUS = "2";
const char* PCA_ADDRESS = "0x40";

class PCA9685 {
private:
    void execute(const std::string& command) {
        if (system(command.c_str())) {}
    }

public:
    PCA9685(const char* dev, int addr) {
        std::cout << "PCA9685 (i2cset mode) initialized." << std::endl;
    }

    void setFreq(int freq) {
        int prescale = static_cast<int>((25000000.0 / (4096.0 * freq)) - 1.0);
        std::stringstream command;
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x10";
        execute(command.str()); command.str("");
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0xFE " << "0x" << std::hex << prescale;
        execute(command.str()); command.str("");
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x00";
        execute(command.str()); command.str("");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x80";
        execute(command.str());
    }

    void setPWM(int channel, int on, int off) {
        std::stringstream command;
        int on_l = on & 0xFF; int on_h = on >> 8;
        int off_l = off & 0xFF; int off_h = off >> 8;
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x06 + 4 * channel) << " 0x" << on_l;
        execute(command.str()); command.str("");
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x07 + 4 * channel) << " 0x" << on_h;
        execute(command.str()); command.str("");
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x08 + 4 * channel) << " 0x" << off_l;
        execute(command.str()); command.str("");
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x09 + 4 * channel) << " 0x" << off_h;
        execute(command.str());
    }
};

// --- 헬퍼 함수: UI 각도를 PCA9685 펄스 값으로 변환 ---
const double SERVO_MIN_PULSE = 150.0;
const double SERVO_MAX_PULSE = 600.0;

int ui_angle_to_pulse(double ui_angle) {
    // UI 각도(-90 ~ 90)를 물리 각도(0 ~ 180)로 변환
    double servo_angle = std::clamp(ui_angle + 90.0, 0.0, 180.0);
    // 물리 각도를 펄스 값(150 ~ 600)으로 변환
    double pulse = SERVO_MIN_PULSE + (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (servo_angle / 180.0);
    return static_cast<int>(pulse);
}


int main() {
    // --- 1. 초기 서보 설정 ---
    std::map<int, ServoConfig> motor_configs = {
        {15, {15, "1. 베이스", 0.0, -90.0, 90.0}},
        {14, {14, "2. 어깨",   0.0, -90.0, 90.0}},
        {13, {13, "3. 팔꿈치", 0.0, -90.0, 90.0}},
        {12, {12, "4. 손목상하",0.0, -90.0, 90.0}},
        {7,  {7, "5. 손목좌우", 0.0, -90.0, 90.0}},
        {6,  {6, "6. 집게",   0.0, -90.0, 90.0}}
    };

    // --- 2. PCA9685 초기화 ---
    PCA9685 pca(I2C_BUS, std::stoi(PCA_ADDRESS, 0, 16));
    pca.setFreq(50);
    
    // --- 3. 사용자 입력 루프 ---
    while (true) {
        std::cout << "\n--- 현재 서보 설정 ---" << std::endl;
        for (const auto& [ch, config] : motor_configs) {
            std::cout << "채널 " << ch << " (" << config.name << "): standby=" << config.standby_angle
                      << ", min=" << config.min_angle << ", max=" << config.max_angle << std::endl;
        }
        std::cout << "----------------------" << std::endl;

        int channel_num;
        double new_standby, new_move_value;

        std::cout << "설정을 변경할 채널 번호를 입력하세요 (종료: 0): ";
        std::cin >> channel_num;

        if (channel_num == 0) {
            break;
        }
        if (motor_configs.find(channel_num) == motor_configs.end()) {
            std::cerr << "오류: 존재하지 않는 채널 번호입니다." << std::endl;
            continue;
        }

        std::cout << "새로운 Standby 각도를 입력하세요 (-90 ~ 90): ";
        std::cin >> new_standby;
        std::cout << "Standby 기준 좌우 동작 범위(Move Value)를 입력하세요 (예: 30 -> ±30도): ";
        std::cin >> new_move_value;

        // --- 4. 설정 업데이트 ---
        auto& config = motor_configs.at(channel_num);
        config.standby_angle = new_standby;
        config.min_angle = new_standby - new_move_value;
        config.max_angle = new_standby + new_move_value;

        std::cout << "\n>> 채널 " << channel_num << " 업데이트 완료: standby=" << config.standby_angle
                  << ", min=" << config.min_angle << ", max=" << config.max_angle << std::endl;
        std::cout << ">> 새로운 설정으로 움직임을 시연합니다." << std::endl;
        
        // --- 5. 움직임 시연 ---
        std::cout << "1. 새로운 Standby 위치로 이동..." << std::endl;
        pca.setPWM(config.ch, 0, ui_angle_to_pulse(config.standby_angle));
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "2. 새로운 Min 위치로 이동..." << std::endl;
        pca.setPWM(config.ch, 0, ui_angle_to_pulse(config.min_angle));
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "3. 새로운 Max 위치로 이동..." << std::endl;
        pca.setPWM(config.ch, 0, ui_angle_to_pulse(config.max_angle));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        std::cout << "4. Standby 위치로 복귀..." << std::endl;
        pca.setPWM(config.ch, 0, ui_angle_to_pulse(config.standby_angle));
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <map>
#include <mutex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace cv;
using namespace std;

// --- 구조체 정의 (main.rs 참고) ---
struct ServoConfig {
    int ch;
    string name;
    double standby_angle; // UI 기준 각도 (-90 ~ 90)
    double min_angle;
    double max_angle;
    bool invert;
};

struct ServoState {
    bool on = true;
    double current_angle = 90.0; // 서보 실제 각도 (0 ~ 180)
    double target_angle = 90.0;
    double speed = 50.0; // deg/sec
};

// --- 설정값 (index.html 및 main.rs 참고) ---
const double ARM_L1 = 105.0; // 어깨->팔꿈치 길이(mm)
const double ARM_L2 = 80.0;  // 팔꿈치->손목 길이(mm)
const char* I2C_DEV = "/dev/i2c-2";
const int PCA_ADDR = 0x40;
const int SERVO_FREQ = 50;
const double SERVO_MIN_PULSE = 150.0;
const double SERVO_MAX_PULSE = 600.0;
const Size CHECKER_PATTERN = {7, 6}; // 8x7 체커보드의 내부 교차점
const double SQUARE_MM = 30.0;
const Point2d ROBOT_BASE_MM = {0.0, -120.0};

// --- 전역 변수: 상태 및 설정을 스레드 간 공유 ---
map<int, ServoConfig> motor_configs;
map<int, ServoState> motor_states;
mutex states_mutex; // 상태 접근을 위한 뮤텍스

const char* I2C_BUS = "2";
const char* PCA_ADDRESS = "0x40";

class PCA9685 {
private:
    // i2cset 명령어를 실행하는 헬퍼 함수
    void execute(const std::string& command) {
        // system() 함수의 반환값을 무시하여 경고를 방지합니다.
        if (system(command.c_str())) {
            std::cout << "Error: Command failed - " << command << std::endl;
        }
    }

public:
    PCA9685(const char* dev, int addr) {
        std::cout << "PCA9685 (i2cset mode) initialized." << std::endl;
    }

    // 데이터시트 규격에 맞는 안정적인 주파수 설정
    void setFreq(int freq) {
        int prescale = static_cast<int>((25000000.0 / (4096.0 * freq)) - 1.0);
        
        std::stringstream command;
        
        // 1. Sleep 모드로 진입
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x10";
        execute(command.str());
        command.str(""); // 스트림 비우기

        // 2. 주파수(prescale) 설정
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0xFE " << "0x" << std::hex << prescale;
        execute(command.str());
        command.str("");

        // 3. Wake 모드로 전환
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x00";
        execute(command.str());
        command.str("");

        // 4. 오실레이터 안정화 대기 (5ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // 5. PWM 출력 재시작
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x00 0x80";
        execute(command.str());
    }

    // PWM 값 설정
    void setPWM(int channel, int on, int off) {
        std::stringstream command;
        int on_l = on & 0xFF;
        int on_h = on >> 8;
        int off_l = off & 0xFF;
        int off_h = off >> 8;

        // ON Low Byte
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x06 + 4 * channel) << " 0x" << on_l;
        execute(command.str());
        command.str("");
        
        // ON High Byte
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x07 + 4 * channel) << " 0x" << on_h;
        execute(command.str());
        command.str("");

        // OFF Low Byte
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x08 + 4 * channel) << " 0x" << off_l;
        execute(command.str());
        command.str("");

        // OFF High Byte
        command << "sudo i2cset -y " << I2C_BUS << " " << PCA_ADDRESS << " 0x" << std::hex << (0x09 + 4 * channel) << " 0x" << off_h;
        execute(command.str());
    }
};

// ---------------- 함수들 (main.rs 참고) ----------------
double ui_angle_to_servo_angle(double ui_angle) {
    return clamp(ui_angle + 90.0, 0.0, 180.0);
}
int servo_angle_to_pulse(double angle) {
    double angle_clamped = clamp(angle, 0.0, 180.0);    
    double pulse = SERVO_MIN_PULSE + (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle_clamped / 180.0);
    return int(pulse);
}
double rad2deg(double r){ return r * 180.0 / M_PI; }
double deg2rad(double d){ return d * M_PI / 180.0; }
double clamp(double v, double min_v, double max_v) {
    return std::max(min_v, std::min(max_v, v));
}

// ---------------- 서보 제어 스레드 (main.rs 참고) ----------------
void servo_control_thread_func(PCA9685* pca) {
    auto update_interval = chrono::milliseconds(20);
    while (true) {
        lock_guard<mutex> lock(states_mutex);
        for (auto const& [ch_num, config] : motor_configs) {
            if (motor_states.count(ch_num)) {
                auto& state = motor_states.at(ch_num);
                if (state.on) {
                    double diff = state.target_angle - state.current_angle;
                    if (abs(diff) > 0.1) {
                        double max_delta = state.speed * 0.020; // 20ms
                        state.current_angle += clamp(diff, -max_delta, max_delta);
                        int pulse = servo_angle_to_pulse(state.current_angle);
                        pca->setPWM(ch_num, 0, pulse);
                    }
                }
            }
        }
        this_thread::sleep_for(update_interval);
    }
}

bool solve_ik(double X, double Y, double &base_deg, double &shoulder_deg, double &elbow_deg) {
    base_deg = rad2deg(atan2(Y, X));
    double r = sqrt(X*X + Y*Y);
    if (r > (ARM_L1 + ARM_L2) || r < abs(ARM_L1 - ARM_L2)) {
        cerr << "IK Error: Target is unreachable (r=" << r << "mm)\n";
        return false;
    }
    double cos_theta2 = (r*r - ARM_L1*ARM_L1 - ARM_L2*ARM_L2) / (2.0 * ARM_L1 * ARM_L2);
    cos_theta2 = clamp(cos_theta2, -1.0, 1.0); 
    double theta2_rad = acos(cos_theta2);
    elbow_deg = 180.0 - rad2deg(theta2_rad);
    double theta1_rad = atan2(ARM_L2 * sin(theta2_rad), ARM_L1 + ARM_L2 * cos(theta2_rad));
    shoulder_deg = rad2deg(theta1_rad);
    return true;
}

bool calibrate_camera(VideoCapture &cap, double &mm_per_px, Point2d &board_center_px) {
    Mat frame;

    // 여러 프레임을 읽어 카메라가 안정화될 시간을 줍니다.
    for (int i = 0; i < 30; ++i) {
        if (!cap.read(frame)) {
            cerr << "프레임 캡처 실패 (워밍업 중)\n";
            return false;
        }
        this_thread::sleep_for(chrono::milliseconds(100)); // 프레임 간 짧은 딜레이
    }
    
    if (frame.empty()) {
        cerr << "캡처된 프레임이 비어있습니다.\n";
        return false;
    }
    
    // 디버깅을 위해 현재 프레임을 파일로 저장합니다.
    imwrite("debug_frame.jpg", frame);
    cout << "디버깅용 이미지 'debug_frame.jpg'를 저장했습니다." << endl;

    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    vector<Point2f> corners;
    
    // findChessboardCorners 호출
    bool found = findChessboardCorners(gray, CHECKER_PATTERN, corners);

    if (!found) { 
        cerr << "Checkerboard not found." << endl; 
        return false;
    }
    
    cornerSubPix(gray, corners, Size(11,11), Size(-1,-1), TermCriteria(TermCriteria::EPS+TermCriteria::MAX_ITER, 30, 0.001));
    double dist_sum = 0;
    int dist_count = 0;
    for (size_t i = 0; i < corners.size() - 1; ++i) {
        if (i % CHECKER_PATTERN.width != CHECKER_PATTERN.width - 1) {
            dist_sum += norm(corners[i] - corners[i+1]);
            dist_count++;
        }
    }
    if (dist_count == 0) return false;
    mm_per_px = SQUARE_MM / (dist_sum / dist_count);
    Point2f center_sum(0,0);
    for(const auto& p : corners) center_sum += p;
    board_center_px = Point2d(center_sum.x / corners.size(), center_sum.y / corners.size());
    return true;
}

bool detect_red_centroid(const Mat &frame, Point &centroid) {
    Mat hsv, mask1, mask2, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);
    inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mask1);
    inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mask2);
    mask = mask1 | mask2;
    erode(mask, mask, Mat(), Point(-1,-1), 2);
    dilate(mask, mask, Mat(), Point(-1,-1), 2);
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return false;
    double max_area = 0;
    int max_idx = -1;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area > max_area) {
            max_area = area;
            max_idx = (int)i;
        }
    }
    if (max_idx == -1) return false;
    Moments M = moments(contours[max_idx]);
    if (M.m00 == 0) return false;
    centroid = Point(M.m10 / M.m00, M.m01 / M.m00);
    return true;
}

int main() {
    // --- 1. 설정 초기화 ---
    motor_configs = {
        {15, {15, "1. 베이스 회전", -35.0, -90.0, 35.0, true}},
        {14, {14, "2. 어깨", -30.0, -50.0, 15.0, false}},
        {13, {13, "3. 팔꿈치", -10.0, -20.0, 30.0, false}},
        {12, {12, "4. 손목 상하", -45.0, -50.0, 30.0, true}}, // 상하인데 좌우로 반전되는것은 특이 케이스. 일단 invert 적용.
        {7,  {7, "5. 손목 좌우", -10.0, -80.0, 80.0, true}},
        {6,  {6, "6. 집게", 10.0, -15.0, 35.0, false}}
    };
    
    // --- 2. PCA9685 초기화 ---
    PCA9685 pca(I2C_DEV, PCA_ADDR);
    // if (!pca.is_ok()) { 
    //     cerr << "PCA9685 초기화 실패\n";
    //     return -1;
    // }
    pca.setFreq(SERVO_FREQ);
    //pca.enable();

    // --- 3. 초기 대기 자세 설정 ---
    {
        lock_guard<mutex> lock(states_mutex);
        cout << "--- 초기 서보 각도 설정 ---" << endl; // 디버깅 라인 추가
        for (auto const& [ch_num, config] : motor_configs) {
            double standby_angle_ui = config.standby_angle;
            double initial_servo_angle = ui_angle_to_servo_angle(config.invert ? -standby_angle_ui : standby_angle_ui);

            cout << config.name << " (Ch " << ch_num << "): Standby " 
             << standby_angle_ui << " -> Servo Angle " << initial_servo_angle << endl;


            motor_states[ch_num] = ServoState { 
                true,
                initial_servo_angle,
                initial_servo_angle,
                (10.0 / 100.0) * 720.0 + 10.0 
            };
            pca.setPWM(ch_num, 0, servo_angle_to_pulse(initial_servo_angle));
        }
        cout << "--------------------------" << endl; // 디버깅 라인 추가
    }
    cout << "모든 서보가 대기 자세로 이동했습니다.\n";
    this_thread::sleep_for(chrono::seconds(2));

    // --- 4. 백그라운드 서보 제어 스레드 시작 ---
    thread servo_thread(servo_control_thread_func, &pca);
    servo_thread.detach();

    // --- 5. 비전 작업 시작 ---
    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    this_thread::sleep_for(chrono::milliseconds(200));
    
    double mm_per_px;
    Point2d board_center_px;
    if (!calibrate_camera(cap, mm_per_px, board_center_px)) return -1;

    Mat frame;
    if (!cap.read(frame)) { cerr << "프레임 캡처 실패\n"; return -1; }

    Point object_centroid_px;
    if (!detect_red_centroid(frame, object_centroid_px)) {
        cerr << "빨간 물체를 찾지 못했습니다.\n"; return -1; }

    // --- 6. 좌표계 변환 ---
    double world_x_mm = (board_center_px.y - object_centroid_px.y) * mm_per_px;
    double world_y_mm = (object_centroid_px.x - board_center_px.x) * mm_per_px;
    double robot_x_mm = world_x_mm - ROBOT_BASE_MM.x;
    double robot_y_mm = world_y_mm - ROBOT_BASE_MM.y;

    // --- 7. IK 계산 ---
    double base_deg, shoulder_deg, elbow_deg;
    if (!solve_ik(robot_x_mm, robot_y_mm, base_deg, shoulder_deg, elbow_deg)) return -1;

    // --- 8. 목표 상태 업데이트 ---
    cout << "IK 목표 각도 계산 완료. 제어 스레드로 목표 전달...\n";
    {
        lock_guard<mutex> lock(states_mutex);
        
        // 베이스 (채널 15)
        auto& base_cfg = motor_configs.at(15);
        double base_target_ui = base_cfg.invert ? -base_deg : base_deg;
        motor_states[15].target_angle = ui_angle_to_servo_angle(clamp(base_target_ui, base_cfg.min_angle, base_cfg.max_angle));
        motor_states[15].on = true;

        // 어깨 (채널 14)
        auto& shoulder_cfg = motor_configs.at(14);
        double shoulder_target_ui = shoulder_cfg.invert ? -shoulder_deg : shoulder_deg;
        motor_states[14].target_angle = ui_angle_to_servo_angle(clamp(shoulder_target_ui, shoulder_cfg.min_angle, shoulder_cfg.max_angle));
        motor_states[14].on = true;
        
        // 팔꿈치 (채널 13)
        auto& elbow_cfg = motor_configs.at(13);
        double elbow_target_ui = elbow_cfg.invert ? -elbow_deg : elbow_deg;
        motor_states[13].target_angle = ui_angle_to_servo_angle(clamp(elbow_target_ui, elbow_cfg.min_angle, elbow_cfg.max_angle));
        motor_states[13].on = true;
    }
    
    cout << "움직임 시작. 5초 후 종료합니다.\n";
    this_thread::sleep_for(chrono::seconds(5));

    cout << "작업 완료.\n";
    return 0;
}
// Robot Arm Servo Configuration Tool (Refactored)
// PCA9685 I2C control: ioctl + i2c_smbus_write_i2c_block_data 
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
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

using namespace std;
using namespace cv;

//==========================================================================================================
// Motor Control Code
//==========================================================================================================
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

void grab_open_motion() {
    //open channel 6 : angle 65
    std::cout << ">> grab open..." << std::endl;
    smooth_move(pca, current_angles, 6, 65.0, 100.0); //open channel 6 : angle 65
}
void grab_close_motion() {
    //close channel 6 : angle 115
    std::cout << ">> grab close..." << std::endl;
    smooth_move(pca, current_angles, 6, 94.0, 100.0); //close channel 6 : angle 94 smalbox
}

int motor_test() {
    // --- 1. 초기 서보 설정 ---
    std::map<int, ServoConfig> motor_configs_zero = {
        {15, {15, "1. 베이스",   90.0, 60.0, 120.0}},
        {14, {14, "2. 어깨",     90.0, 60.0, 120.0}},
        {13, {13, "3. 팔꿈치",   90.0, 60.0, 120.0}},
        {12, {12, "4. 손목상하", 120.0, 90.0, 150.0}},
        {7,  {7, "5. 손목좌우",  90.0, 60.0, 120.0}},
        {6,  {6, "6. 집게",      90.0, 60.0, 120.0}}
    };

    std::map<int, ServoConfig> motor_configs = {
        {15, {15, "1. 베이스",   95.0,  30.0, 160.0}},  //min : right, max : left
        {14, {14, "2. 어깨",     110.0,  30.0, 130.0}}, //min : down, max : up
        {13, {13, "3. 팔꿈치",   90.0, 50.0, 160.0}},   //min : down, max : up
        {12, {12, "4. 손목상하",  130.0, 30.0, 170.0}}, //min : up, max : down
        {7,  {7, "5. 손목좌우",   90.0, 0.0, 180.0}},   //min : left, max : right
        {6,  {6, "6. 집게",       90.0, 65.0, 115.0}}   //min : open, max : grab(close)
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
        //pca.setPWM(config.ch, 0, angle_to_pulse(config.standby_angle));
        smooth_move(pca, motor_states, config.ch, config.standby_angle, motor_speed);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (const auto& [ch, config] : motor_configs) {
        motor_states[ch] = config.standby_angle;
        smooth_move(pca, motor_states, config.ch, config.standby_angle, motor_speed);
        //pca.setPWM(config.ch, 0, angle_to_pulse(config.standby_angle));
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
                      << ", current=" << motor_states[ch]
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

//==========================================================================================================
// Vision Test Code
// ---------------- utility ----------------
double rad2deg(double r){ return r * 180.0 / M_PI; }
double deg2rad(double d){ return d * M_PI / 180.0; }
double clampd(double v, double a, double b){ if (v<a) return a; if (v>b) return b; return v; }

const double ARM_L1 = 105.0;
const double ARM_L2 = 80.0;
const int CHECKER_SQUARE_X = 8; // 가로 Cell 개수
const int CHECKER_SQUARE_Y = 7; // 세로 Cell 개수
const double CHECKER_SQUARE_MM = 30.0;
const Size CHECKER_PATTERN(CHECKER_SQUARE_X - 1, CHECKER_SQUARE_Y - 1); // 내부 코너 개수
const cv::Point2d ROBOT_BASE_MM = {0.0, -120.0}; // 로봇 기준 좌표
struct CheckerConfig {
    int squares_x = CHECKER_SQUARE_X;
    int squares_y = CHECKER_SQUARE_Y;
    double square_mm = CHECKER_SQUARE_MM;
} cfg;

// ---------------- IK (simple planar 2-link) ----------------
// Returns true if reachable, outputs base_deg, shoulder_deg, elbow_deg (degrees)
bool planar_ik(double X, double Y, double &base_deg, double &shoulder_deg, double &elbow_deg) {
    // base yaw: angle to target in XY plane
    base_deg = rad2deg(atan2(Y, X));

    // convert to planar distance r
    double r = sqrt(X*X + Y*Y);
    double L1 = ARM_L1, L2 = ARM_L2;
    // check reachability
    double maxreach = L1 + L2;
    if (r > maxreach) return false;

    // law of cos for elbow angle
    double cos_theta2 = (r*r - L1*L1 - L2*L2) / (2.0 * L1 * L2);
    cos_theta2 = clampd(cos_theta2, -1.0, 1.0);
    double theta2 = acos(cos_theta2); // elbow angle (rad)

    // shoulder angle
    double k1 = L1 + L2 * cos(theta2);
    double k2 = L2 * sin(theta2);
    double theta1 = atan2(Y, X) - atan2(k2, k1);

    shoulder_deg = rad2deg(theta1);
    elbow_deg = rad2deg(theta2);

    return true;
}

// ---------------- compute pixel->mm using chessboard ----------------
bool compute_pixelscale_and_origin(VideoCapture &cap, double &mm_per_px, double &pixels_per_square, Point2d &origin_px) {
    // capture one frame from camera
    Mat frame;
    if (!cap.read(frame)) {
        cerr << "Failed to capture frame for checker measurement\n";
        return false;
    }

    int inner_corners_x = cfg.squares_x - 1; // 7
    int inner_corners_y = cfg.squares_y - 1; // 6
    Size pattern(inner_corners_x, inner_corners_y);

    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);

    vector<Point2f> corners;
    bool found = findChessboardCorners(gray, pattern, corners,
                                       CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE);
    if (!found) {
        cerr << "\nChessboard corners not found. Make sure checker is visible and pattern size matches.\n";
        return false;
    }

    // refine
    cornerSubPix(gray, corners, Size(11,11), Size(-1,-1),
                 TermCriteria(TermCriteria::EPS+TermCriteria::MAX_ITER, 30, 0.001));

    // compute average horizontal neighbor distance as pixels_per_square
    vector<double> dists;
    for (int r=0;r<inner_corners_y;r++){
        for (int c=0;c<inner_corners_x-1;c++){
            int idx = r*inner_corners_x + c;
            double d = norm(corners[idx+1] - corners[idx]);
            dists.push_back(d);
        }
    }
    if (dists.empty())
        return false;

    double mean_px = 0;
    for (double v: dists) {
        mean_px += v;
    }
    mean_px /= dists.size();
    pixels_per_square = mean_px;
    mm_per_px = cfg.square_mm / pixels_per_square;

    Point2f bottom_left = corners[(inner_corners_y-1)*inner_corners_x + 0];
    origin_px = bottom_left;

    cout << "\nDetected chessboard. pixels_per_square="<<pixels_per_square
         << " mm_per_px="<<mm_per_px<<"\n";
    cout << "Origin set to bottom-left corner: " << origin_px.x << "," << origin_px.y <<"\n";
    return true;
}

int camera_test()
{
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) { 
        cerr<<"Cannot open camera\n";
        return -1; 
    }
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    this_thread::sleep_for(chrono::milliseconds(100));

    Ptr<aruco::Dictionary> dict = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
    
    Mat frame;
    for (int i = 0; i < 5; i++) {   // 최대 5번까지 시도
        if (cap.read(frame) && !frame.empty()) {
            break;
        }
        std::cerr << "Retry capturing frame...\n";
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    if (frame.empty()) {
        std::cerr << "Camera read failed after retries\n";
        return -1;
    }

    std::vector<int> ids;
    std::vector<std::vector<Point2f>> corners;
    aruco::detectMarkers(frame, dict, corners, ids);

    if(!ids.empty()) {
        aruco::drawDetectedMarkers(frame, corners, ids);

        imwrite("aruco_detected.png", frame);

        for (size_t i = 0; i < ids.size(); i++) {
            Point2f c(0, 0);
            for (auto &p : corners[i]) c += p;
            c *= 0.25;
            std::cout << "Marker " << ids[i]
                    << " centroid: (" << c.x << "," << c.y << ")\n";
        }
    } else {
        std::cout << "No markers detected.\n";
    }
    return 0;
}

//==========================================================================================================
int main() {
    // motor test or camera test
    int mode = get_user_input<int>("Select mode: \n1. Motor Test  \n2. Camera Test : ");
    if (mode == 1) {
        return motor_test();
    } else if (mode == 2) {
        return camera_test();
    } else {
        std::cerr << "Invalid mode selected. Exiting." << std::endl;
        return -1;
    }
}



// //==========================  CHECKERBOARD method FAIL!!! ===========================
// // ---------------- detect red centroid ----------------
// bool detect_red_centroid(const Mat &frame, Point &centroid) {
//     Mat hsv;
//     cvtColor(frame, hsv, COLOR_BGR2HSV);

//     Mat mask1, mask2;
//     inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mask1);
//     inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mask2);
//     Mat mask = mask1 | mask2;

//     // morphology
//     erode(mask, mask, Mat(), Point(-1,-1), 2);
//     dilate(mask, mask, Mat(), Point(-1,-1), 2);

//     vector<vector<Point>> contours;
//     findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
//     if (contours.empty()) return false;

//     // largest contour
//     double bestA = 0; int bi = -1;
//     for (size_t i=0;i<contours.size();++i) {
//         double a = contourArea(contours[i]);
//         if (a > bestA) { bestA = a; bi = (int)i; }
//     }
//     if (bi < 0) return false;
//     Moments M = moments(contours[bi]);
//     if (M.m00 <= 0) return false;
//     centroid.x = int(M.m10 / M.m00);
//     centroid.y = int(M.m01 / M.m00);
//     return true;
// }

// int camera_test() {
//     // try load camera calibration if available (YAML)
//     Mat cameraMatrix, distCoeffs;
//     bool haveCalibration = false;
//     FileStorage fs("calib.yml", FileStorage::READ);
//     if (fs.isOpened()) {
//         fs["camera_matrix"] >> cameraMatrix;
//         fs["dist_coeffs"] >> distCoeffs;
//         haveCalibration = !cameraMatrix.empty();
//         std::cout << "Loaded calib.yml" << std::endl;
//     } else {
//         std::cout << "No calib.yml found; proceeding without undistort." << std::endl;
//     }

//     VideoCapture cap(0, CAP_V4L2);
//     if (!cap.isOpened()) { 
//         cerr<<"Cannot open camera\n";
//         return -1; 
//     }
//     cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
//     cap.set(CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(CAP_PROP_FRAME_HEIGHT, 480);
//     this_thread::sleep_for(chrono::milliseconds(200));
//     int value;

//     while(true) {
//         std::cout << "\n--- New Capture Sequence ---\n";
//         // hold for a moment until user input
//         std::cout << "Enter 1 to capture and process, 0 to exit: ";
//         std::cin >> value;
//         if (value == 0) {
//             break;
//         } else if(value == 1) {
//             // 1) compute pixel scaling and origin using chessboard
//             double mm_per_px = 0.0, pixels_per_square = 0.0;
//             Point2d origin_px;
//             bool ok = compute_pixelscale_and_origin(cap, mm_per_px, pixels_per_square, origin_px);
//             if (!ok) {
//                 cerr << "Failed to compute pixel<->mm mapping from checkerboard. Exiting.\n";
//                 return -1;
//             }

//             // 2) capture a fresh frame for object detection
//             Mat frame;
//             if (!cap.read(frame)) {
//                 cerr << "Failed to grab frame\n";
//                 return -1;
//             }

//             // 3) detect red centroid
//             Point centroid_px;
//             if (!detect_red_centroid(frame, centroid_px)) {
//                 cerr << "No red object found in frame\n";
//                 return -1;
//             }
//             cout << "Pixel centroid: ("<<centroid_px.x<<","<<centroid_px.y<<")\n";

//             // 4) pixel -> robot mm
//             double Xmm = (centroid_px.x - origin_px.x) * mm_per_px; 
//             double Ymm = (centroid_px.y - origin_px.y) * mm_per_px;
//             cout << "Target (mm): X="<<Xmm<<" Y="<<Ymm<<"\n";

//             // 5) IK
//             double base_deg, sh_deg, el_deg;
//             if (!planar_ik(Xmm, Ymm, base_deg, sh_deg, el_deg)) {
//                 cout << "Target unreachable by IK\n";
//                 return 0;
//             }
//             cout << "IK result (deg): base="<<base_deg<<" shoulder="<<sh_deg<<" elbow="<<el_deg<<"\n";
//         }
//     }
//     return 0;
// }

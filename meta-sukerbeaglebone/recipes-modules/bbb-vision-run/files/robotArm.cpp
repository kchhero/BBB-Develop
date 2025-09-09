#include "robotArm.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <map>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits> 
#include <algorithm>

using namespace std;
using namespace cv;

const Size CHECKER_PATTERN = {7, 6}; // 8x7 체커보드의 내부 교차점
const double SQUARE_MM = 30.0;

RobotArm::RobotArm() {}
RobotArm::~RobotArm() {}

void RobotArm::smoothMove(PCA9685& pca, std::map<int, double>& current_angles,
                 int channel, double target_angle,
                 bool initial_move = false)
{
    const double MAX_SERVO_SPEED_DPS = 540.0;
    const double speed_dps = MAX_SERVO_SPEED_DPS * SPEED_FACTOR;

    double start_angle = current_angles.at(channel);
    double total_distance = target_angle - start_angle;
    if (!initial_move && std::abs(total_distance) < 0.1) 
        return;

    // MG996R 최대 속도 기준
    // step당 1 deg로 나누고, 이동 시간 계산
    int total_steps = static_cast<int>(std::ceil(std::abs(total_distance)));
    if (total_steps < 1) {
        total_steps = 1;
    }

    double total_duration_s = std::abs(total_distance) / speed_dps;
    auto delay_per_step = std::chrono::milliseconds(
        std::max(1L, static_cast<long>((total_duration_s * 1000.0) / total_steps))
    );

    for (int step = 1; step <= total_steps; ++step) {
        double progress = static_cast<double>(step) / total_steps;
        // ease-in-out
        double ease_factor = (1.0 - std::cos(progress * M_PI)) / 2.0;
        double current_angle = start_angle + total_distance * ease_factor;
        pca.setPWM(channel, 0, pca.angle_to_pulse(current_angle));
        std::this_thread::sleep_for(delay_per_step);
    }

    current_angles[channel] = target_angle;
}

void RobotArm::move_to_target(PCA9685& pca,
                    std::map<int, double>& current_angles,
                    const ServoConfig& config,
                    double target_angle)
{
    if (target_angle < config.min_angle) target_angle = config.min_angle;
    if (target_angle > config.max_angle) target_angle = config.max_angle;

    smoothMove(pca, current_angles, config.ch, target_angle, false);
}

bool RobotArm::initialize() {
    this->motor_configs = {
        {15, {15, "1. 베이스",   95.0,  30.0, 160.0}},
        {14, {14, "2. 어깨",     90.0,  20.0, 100.0}},
        //{13, {13, "3. 팔꿈치",   100.0, 40.0, 150.0}},
        {12, {12, "4. 손목상하",  130.0, 30.0, 170.0}},
        {7,  {7, "5. 손목좌우",   90.0, 0.0, 180.0}},
        // {6,  {6, "6. 집게",       90.0, 0.0, 180.0}}
    };

    pca = std::make_unique<PCA9685>("/dev/i2c-2", 0x40);
    if (!pca->isOk()) {
        return false;
    }
    pca->setFreq(50);

    // 초기 상태 설정
    for (const auto& [ch, config] : motor_configs) {
        current_angles[ch] = config.standby_angle;
        pca->setPWM(ch, 0, pca->angle_to_pulse(config.standby_angle));
    }
    
    std::cout << "RobotArm initialized." << std::endl;
    return true;
}

void RobotArm::setGripper(bool grab) {
    //smoothMove(6, grab ? motor_configs.at(6).max_angle : motor_configs.at(6).min_angle);
}

void RobotArm::pickAndPlace(const cv::Point& object_centroid_px) {
    double world_x = (board_center_px.y - object_centroid_px.y) * mm_per_px;
    double world_y = (object_centroid_px.x - board_center_px.x) * mm_per_px;
    double robot_x = world_x - ROBOT_BASE_MM.x;
    double robot_y = world_y - ROBOT_BASE_MM.y;

    double base, shoulder, elbow;
    if (!solveIK(robot_x, robot_y, base, shoulder, elbow)) return;
    
    std::cout << "1. 대기 자세로 이동..." << std::endl;
    // smoothMoveMultiple({
    //     {14, motor_configs.at(14).standby_angle}, 
    //     {13, motor_configs.at(13).standby_angle},
    //     {12, motor_configs.at(12).standby_angle}
    // });
    
    // std::cout << "2. 물체 위로 이동..." << std::endl;
    // smoothMoveMultiple({{15, base}, {14, shoulder}, {13, elbow}});
    
    // std::cout << "3. 집게 열기..." << std::endl;
    // setGripper(false);
    
    // std::cout << "4. 물체 잡으러 내려가기..." << std::endl;
    // smoothMove(12, 150.0); // 손목을 숙임
    
    // std::cout << "5. 물체 잡기..." << std::endl;
    // setGripper(true);
    
    // std::cout << "6. 팔 들어올리기..." << std::endl;
    // smoothMove(12, motor_configs.at(12).standby_angle); // 손목을 다시 올림
    
    // std::cout << "7. 놓는 위치로 이동..." << std::endl;
    // smoothMoveMultiple({{15, 30.0}, {14, 90.0}, {13, 100.0}});
    
    // std::cout << "8. 물체 놓기..." << std::endl;
    // setGripper(false);
    
    // std::cout << "9. 초기 자세로 복귀..." << std::endl;
    // smoothMoveMultiple({
    //     {15, motor_configs.at(15).standby_angle},
    //     {14, motor_configs.at(14).standby_angle},
    //     {13, motor_configs.at(13).standby_angle},
    //     {12, motor_configs.at(12).standby_angle}
    // });
}

bool RobotArm::solveIK(double X, double Y, double& base, double& shoulder, double& elbow) {
    // base_deg = rad2deg(atan2(Y, X));
    // double r = sqrt(X*X + Y*Y);
    // if (r > (ARM_L1 + ARM_L2) || r < abs(ARM_L1 - ARM_L2)) {
    //     cerr << "IK Error: Target is unreachable (r=" << r << "mm)\n";
    //     return false;
    // }
    // double cos_theta2 = (r*r - ARM_L1*ARM_L1 - ARM_L2*ARM_L2) / (2.0 * ARM_L1 * ARM_L2);
    // cos_theta2 = std::clamp(cos_theta2, -1.0, 1.0); 
    // double theta2_rad = acos(cos_theta2);
    // elbow_deg = 180.0 - rad2deg(theta2_rad);
    // double theta1_rad = atan2(ARM_L2 * sin(theta2_rad), ARM_L1 + ARM_L2 * cos(theta2_rad));
    // shoulder_deg = rad2deg(theta1_rad);
    return true;
}

bool RobotArm::calibrateCamera() { 
    // Mat frame;

    // // 여러 프레임을 읽어 카메라가 안정화될 시간을 줍니다.
    // for (int i = 0; i < 30; ++i) {
    //     if (!cap.read(frame)) {
    //         cerr << "프레임 캡처 실패 (워밍업 중)\n";
    //         return false;
    //     }
    //     this_thread::sleep_for(chrono::milliseconds(100)); // 프레임 간 짧은 딜레이
    // }
    
    // if (frame.empty()) {
    //     cerr << "캡처된 프레임이 비어있습니다.\n";
    //     return false;
    // }
    
    // // 디버깅을 위해 현재 프레임을 파일로 저장합니다.
    // imwrite("debug_frame.jpg", frame);
    // cout << "디버깅용 이미지 'debug_frame.jpg'를 저장했습니다." << endl;

    // Mat gray;
    // cvtColor(frame, gray, COLOR_BGR2GRAY);
    // vector<Point2f> corners;
    
    // // findChessboardCorners 호출
    // bool found = findChessboardCorners(gray, CHECKER_PATTERN, corners);

    // if (!found) { 
    //     cerr << "Checkerboard not found." << endl; 
    //     return false;
    // }
    
    // cornerSubPix(gray, corners, Size(11,11), Size(-1,-1), TermCriteria(TermCriteria::EPS+TermCriteria::MAX_ITER, 30, 0.001));
    // double dist_sum = 0;
    // int dist_count = 0;
    // for (size_t i = 0; i < corners.size() - 1; ++i) {
    //     if (i % CHECKER_PATTERN.width != CHECKER_PATTERN.width - 1) {
    //         dist_sum += norm(corners[i] - corners[i+1]);
    //         dist_count++;
    //     }
    // }
    // if (dist_count == 0) return false;
    // mm_per_px = SQUARE_MM / (dist_sum / dist_count);
    // Point2f center_sum(0,0);
    // for(const auto& p : corners) center_sum += p;
    // board_center_px = Point2d(center_sum.x / corners.size(), center_sum.y / corners.size());
    return true;
}

bool RobotArm::detectObject(cv::Point& centroid) {
    // Mat hsv, mask1, mask2, mask;
    // cvtColor(frame, hsv, COLOR_BGR2HSV);
    // inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mask1);
    // inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mask2);
    // mask = mask1 | mask2;
    // erode(mask, mask, Mat(), Point(-1,-1), 2);
    // dilate(mask, mask, Mat(), Point(-1,-1), 2);
    // vector<vector<Point>> contours;
    // findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    // if (contours.empty()) return false;
    // double max_area = 0;
    // int max_idx = -1;
    // for (size_t i = 0; i < contours.size(); i++) {
    //     double area = contourArea(contours[i]);
    //     if (area > max_area) {
    //         max_area = area;
    //         max_idx = (int)i;
    //     }
    // }
    // if (max_idx == -1) return false;
    // Moments M = moments(contours[max_idx]);
    // if (M.m00 == 0) return false;
    // centroid = Point(M.m10 / M.m00, M.m01 / M.m00);
    return true;
}

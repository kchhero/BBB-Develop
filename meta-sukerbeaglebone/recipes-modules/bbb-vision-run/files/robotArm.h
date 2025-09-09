#ifndef ROBOT_ARM_H
#define ROBOT_ARM_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <mutex>
#include <cmath>

#include "pca9685.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPEED_FACTOR 0.2 //Maximum speed factor is 0.2 ~ 0.3

struct ServoConfig {
    int ch;
    std::string name;
    double standby_angle;
    double min_angle;
    double max_angle;
};

class RobotArm {
public:
    RobotArm();
    ~RobotArm();

    bool initialize();
    bool calibrateCamera();
    bool detectObject(cv::Point& centroid);
    void pickAndPlace(const cv::Point& object_centroid_px);

private:
    void move_to_target(PCA9685& pca, std::map<int, double>& current_angles, const ServoConfig& config, double target_angle);
    void smoothMove(PCA9685& pca, std::map<int, double>& current_angles, int channel, double target_angle, bool initial_move);
    void setGripper(bool grab);
    bool solveIK(double x, double y, double& base, double& shoulder, double& elbow);
    
    std::unique_ptr<PCA9685> pca;
    cv::VideoCapture cap;
    
    std::map<int, ServoConfig> motor_configs;
    std::map<int, double> current_angles; // 서보의 현재 각도를 계속 추적

    // 보정 및 좌표 데이터
    double mm_per_px;
    cv::Point2d board_center_px;
    const double ARM_L1 = 105.0;
    const double ARM_L2 = 80.0;
    const cv::Point2d ROBOT_BASE_MM = {0.0, -120.0};
};

#endif // ROBOT_ARM_H
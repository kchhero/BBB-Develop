
#include "robotArm.h"

int main() {
    RobotArm arm;
    if (!arm.initialize()) {
        return -1;
    }
    
    // 이 부분은 실제 카메라/객체 탐지 로직으로 대체 필요
    // if (!arm.calibrateCamera()) return -1;
    cv::Point object_location(320, 240); // 임시 객체 위치
    // if (!arm.detectObject(object_location)) return -1;

    arm.pickAndPlace(object_location);
    
    std::cout << "All tasks completed." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
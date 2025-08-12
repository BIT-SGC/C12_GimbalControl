#include "gimbal_ctrl.h"
#include <thread>

int main(int argc, char *argv[]) {
    std::string target_ip = "192.168.144.108";
    uint16_t port = 5000;

    GimbalCtrl gimbal_ctrl(target_ip, port);

    gimbal_ctrl.setGimbalAngle(90, 90, 90, 90);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    gimbal_ctrl.setThermalColorMode(GimbalCtrl::ColorMode::NIGHT);
    gimbal_ctrl.getFirmwareVersion();
    // gimbal_ctrl.setInstallMode(GimbalCtrl::InstallMode::REVERSE);
    // gimbal_ctrl.setGimbalSpeed(10, 30);
    for(uint8_t i = 90; i > 0; i--) {
        gimbal_ctrl.setGimbalSpeed(90, 90);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    gimbal_ctrl.setGimbalAngle(0, 0, 0, 90);
    gimbal_ctrl.controlRecording(GimbalCtrl::RecordState::START);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    gimbal_ctrl.queryRecordingStatus();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    gimbal_ctrl.controlRecording(GimbalCtrl::RecordState::STOP);
    gimbal_ctrl.queryRecordingStatus();
    gimbal_ctrl.capturePhoto();
    gimbal_ctrl.setThermalColorMode(GimbalCtrl::ColorMode::BLACK_HOT);

    return 0;
}
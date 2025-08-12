#ifndef __GIMBAL_CTRL_H__
#define __GIMBAL_CTRL_H__

#include "loguru/loguru.hpp"
#include "practical_socket/PracticalSocket.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

class GimbalCtrl {
public:
  // 云台动作枚举
  enum class GimbalAction : uint8_t {
    STOP = 0x00,
    UP = 0x01,
    DOWN = 0x02,
    LEFT = 0x03,
    RIGHT = 0x04,
    CENTER = 0x05,
    FOLLOW_MODE = 0x06,
    LOCK_MODE = 0x07,
    TOGGLE_MODE = 0x08,
    CALIBRATE = 0x09,
    CEILING_MOUNT = 0x0A,
    INVERTED_MOUNT = 0x0B,
    LEVEL_CALIB = 0x0C,
    VERTICAL_CALIB = 0x0D
  };

  // 录像状态枚举
  enum class RecordState : uint8_t { STOP = 0x00, START = 0x01, TOGGLE = 0x0A };

  // 缩放模式枚举
  enum class ZoomMode : uint8_t {
    ZOOM_1X = 0x00,
    ZOOM_2X = 0x01,
    ZOOM_3X = 0x02,
    ZOOM_4X = 0x03,
    ZOOM_PLUS = 0x0A,
    ZOOM_MINUS = 0x0B
  };

  // 伪彩模式枚举
  enum class ColorMode : uint8_t {
    WHITE_HOT = 1,   // 白热
    SEPIA = 3,       // 辉金
    IRONBOW = 4,     // 铁红
    RAINBOW = 5,     // 彩虹
    NIGHT = 6,       // 微光
    AURORA = 7,      // 极光
    RED_HOT = 8,     // 红热
    JUNGLE = 9,      // 丛林
    MEDICAL = 0xA,   // 医疗
    BLACK_HOT = 0xB, // 黑热
    GOLD_HOT = 0xC   // 金红
  };

  // 安装模式 吊装/倒装
  enum class InstallMode : uint8_t { LIFT = 0x0A, REVERSE = 0x0B };

  // 图像参数结构
  struct ImageParams {
    uint8_t style;      // 0=自定义，1=标准，2=明亮，3=艳丽
    uint8_t hue;        // 色调 0-255
    uint8_t brightness; // 亮度 0-255
    uint8_t saturation; // 饱和度 0-255
    uint8_t contrast;   // 对比度 0-255
    uint8_t sharpness;  // 锐度 0-255
  };

  explicit GimbalCtrl(const std::string &target_ip = "192.168.1.100",
                      uint16_t port = 5000);
  ~GimbalCtrl();

  // 云台控制接口
  bool controlGimbal(GimbalAction action);
  bool setGimbalSpeed(float yaw_speed, float pitch_speed);
  bool setGimbalAngle(float yaw_angle, float pitch_angle, float roll_angle,
                      float speed = 10.0f);

  // 媒体控制接口
  bool controlRecording(RecordState state);
  bool queryRecordingStatus();
  bool capturePhoto();

  // 图像参数接口
  // bool setImageParams(const ImageParams &params);
  // ImageParams getImageParams();

  // 网络配置接口
  bool setNetworkConfig(const std::string &ip, const std::string &gateway);
  std::pair<std::string, std::string> getNetworkConfig();

  // 可见光控制接口
  bool setZoomMode(ZoomMode mode);

  // 热成像控制接口
  // bool setThermalShutter(uint8_t seconds); // TODO
  // uint8_t getThermalShutter();             // TODO
  bool setThermalColorMode(ColorMode mode);

  // 安装模式接口
  bool setInstallMode(InstallMode mode);

  // 系统信息接口
  std::string getFirmwareVersion(); // TODO

  // 错误回调设置
  using ErrorCallback = std::function<void(const std::string &)>;
  void setErrorCallback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
  }

private:
  std::string buildCommand(const std::string &source_addr,
                           const std::string &dest_addr, char control_type,
                           const std::string &identifier,
                           const std::string &data = "");

  std::string buildDynamicCommand(const std::string &source_addr,
                                  const std::string &dest_addr,
                                  char control_type,
                                  const std::string &identifier,
                                  const std::vector<uint8_t> &data);

  std::string buildStaticCommand(const std::string &source_addr,
                                 const std::string &dest_addr,
                                 char control_type,
                                 const std::string &identifier,
                                 const uint8_t &data = 0x00);

  bool send(const std::string &command, int timeout_ms = 1000);
  bool send(const std::string &command, std::string &response,
            int timeout_ms = 1000);
  bool sendAndVerify(const std::string &command);
  uint8_t calculateChecksum(const std::string &frame);
  std::string hexEncode(int32_t value, int num_digits);
  bool waitForData(int timeout_ms);

  // 网络通信成员
  UDPSocket sock_;
  std::string target_ip_;
  uint16_t port_;
  std::mutex socket_mutex_;
  ErrorCallback error_callback_;
};

#endif
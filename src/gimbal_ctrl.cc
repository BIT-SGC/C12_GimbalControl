#include "gimbal_ctrl.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

GimbalCtrl::GimbalCtrl(const std::string &target_ip, uint16_t port)
    : target_ip_(target_ip), port_(port) {
  LOG_F(INFO, "GimbalCtrl init [ip]:%s [port]:%d", target_ip_.c_str(), port_);
}

GimbalCtrl::~GimbalCtrl() {}

// 云台基础控制
bool GimbalCtrl::controlGimbal(GimbalAction action) {
  //   std::string data = hexEncode(static_cast<uint8_t>(action), 2);
  //   std::string cmd = buildCommand("U", "G", 'w', "PTZ", data);
  //   return sendAndVerify(cmd);
  return false;
}

bool GimbalCtrl::setGimbalAngle(float yaw_angle, float pitch_angle,
                                float roll_angle, float speed) {
  yaw_angle = std::max(-90.0f, std::min(90.0f, yaw_angle));
  pitch_angle = std::max(-90.0f, std::min(90.0f, pitch_angle));
  roll_angle = std::max(-90.0f, std::min(90.0f, roll_angle));
  speed = std::max(0.0f, std::min(100.0f, speed));

  uint16_t yaw = static_cast<uint16_t>(yaw_angle * 100);
  uint16_t pitch = static_cast<uint16_t>(pitch_angle * 100);
  uint16_t roll = static_cast<uint16_t>(roll_angle * 100);
  uint8_t _speed = static_cast<uint8_t>(speed); // TODO 确定是否需要 x10

  // 先设置 yaw 角度
  vector<uint8_t> data_buf = {
      static_cast<uint8_t>((yaw >> 8) & 0xFF),
      static_cast<uint8_t>(yaw & 0xFF),
      static_cast<uint8_t>(_speed),
  };
  bool yaw_rtn = send(buildDynamicCommand("U", "G", 'w', "GAY", data_buf), -1);

  // 再设置 pitch 角度
  data_buf.assign(3, 0);
  data_buf = {
      static_cast<uint8_t>((pitch >> 8) & 0xFF),
      static_cast<uint8_t>(pitch & 0xFF),
      static_cast<uint8_t>(_speed),
  };
  bool pitch_rtn =
      send(buildDynamicCommand("U", "G", 'w', "GAP", data_buf), -1);

  // 最后设置 roll 角度
  data_buf.assign(3, 0);
  data_buf = {
      static_cast<uint8_t>((roll >> 8) & 0xFF),
      static_cast<uint8_t>(roll & 0xFF),
      static_cast<uint8_t>(_speed),
  };
  bool roll_rtn = send(buildDynamicCommand("U", "G", 'w', "GAR", data_buf), -1);

  return yaw_rtn && pitch_rtn && roll_rtn;
}

// 云台速度控制 (单位：0.1 deg/s)
bool GimbalCtrl::setGimbalSpeed(float yaw_speed, float pitch_speed) {
  // 限制速度范围 (-127.0 ~ +127.0)
  yaw_speed = std::max(-127.0f, std::min(127.0f, yaw_speed));
  pitch_speed = std::max(-127.0f, std::min(127.0f, pitch_speed));

  // 转换为协议格式 (0.5deg/s 单位)
  int8_t yaw = static_cast<int8_t>(yaw_speed * 2);
  int8_t pitch = static_cast<int8_t>(pitch_speed * 2);

  // std::string yaw_data = hexEncode(yaw, 2);
  // std::string pitch_data = hexEncode(pitch, 2);

  //   分别发送航向和俯仰速度指令
  //   bool yaw_ok = send(buildCommand("U", "G", 'w', "GSY", yaw_data));
  //   bool pitch_ok = send(buildCommand("U", "G", 'w', "GSP", pitch_data));

  bool yaw_ok = send(
      buildStaticCommand("U", "G", 'w', "GSY", {static_cast<uint8_t>(yaw)}),
      -1);
  bool pitch_ok = send(
      buildStaticCommand("U", "G", 'w', "GSP", {static_cast<uint8_t>(pitch)}),
      -1);

  //   return yaw_ok && pitch_ok;
  //   return yaw_ok;
  return false;
}

bool GimbalCtrl::controlRecording(RecordState state) {
  uint8_t data = static_cast<uint8_t>(state);
  std::string cmd = buildStaticCommand("U", "D", 'w', "REC", data);
  LOG_F(INFO, "controlRecording cmd:%s", cmd.c_str());

  // return sendAndVerify(cmd);
  return send(cmd, 999);
}

bool GimbalCtrl::queryRecordingStatus(void) {
  std::string cmd = buildStaticCommand("U", "D", 'r', "REC");
  LOG_F(INFO, "queryRecordingStatus cmd:%s", cmd.c_str());
  // return sendAndVerify(cmd);

  std::string response;
  send(cmd, response, 999);

  // 校验返回值
  // #TPDU2rREC003E 为未录像
  // #TPDU2rREC013F 为正在录像
  if (response == "#TPDU2rREC013F") {
    LOG_F(INFO, "queryRecording status: RECORDING");
    return true;
  }

  LOG_F(INFO, "queryRecording status: IDEL");

  return false;
}

// 拍照
bool GimbalCtrl::capturePhoto() {
  std::string cmd = buildStaticCommand("U", "D", 'w', "CAP", 0x01);
  // return sendAndVerify(cmd);
  return send(cmd, 999);
}

/**
 * @brief 设置缩放模式
 *
 * @param mode
 * @return true
 * @return false
 */
bool GimbalCtrl::setZoomMode(ZoomMode mode) {
  std::string cmd =
      buildStaticCommand("U", "G", 'w', "DZM", {static_cast<uint8_t>(mode)});
  LOG_F(INFO, "setZoomMode cmd:%s", cmd.c_str());
  return send(cmd, 999);
}

/**
 * @brief 设置热成像模式
 *
 * @param mode WHITE_HOT--白热，SEPIA - 辉金，IRONBOW - 铁红，RAINBOW -
 * 彩虹，BLACK_HOT-黑热
 * @return true
 * @return false
 */
bool GimbalCtrl::setThermalColorMode(ColorMode mode) {
  uint8_t data = static_cast<uint8_t>(mode);
  std::string cmd = buildStaticCommand("U", "D", 'w', "IMG", data);

  return send(cmd, 999);
}

/**
 * @brief 设置安装模式
 *
 * @param mode
 * @return true 吊装
 * @return false 竖装
 */
bool GimbalCtrl::setInstallMode(InstallMode mode) {
  std::string cmd =
      buildStaticCommand("U", "G", 'w', "PTZ", {static_cast<uint8_t>(mode)});
  LOG_F(INFO, "setInstallMode cmd:%s", cmd.c_str());

  return send(cmd);
}

std::string GimbalCtrl::getFirmwareVersion() {
  std::string cmd = buildStaticCommand("U", "D", 'r', "VER");
  LOG_F(INFO, "getFirmwareVersion cmd:%s", cmd.c_str());
  send(cmd);
  std::string dummy = " ";
  return dummy;
}

/**
 * @brief 构建不定长命令
 *
 * @param source_addr
 * @param dest_addr
 * @param control_type
 * @param identifier
 * @param data
 * @return std::string
 */
std::string GimbalCtrl::buildCommand(const std::string &source_addr,
                                     const std::string &dest_addr,
                                     char control_type,
                                     const std::string &identifier,
                                     const std::string &data) {
  std::ostringstream cmd;

  // 帧头 + 地址位
  cmd << "#tp" << source_addr << dest_addr;

  // 数据长度 (标识符 3 + 数据长度)
  uint8_t len = static_cast<uint8_t>(3 + data.length());
  cmd << std::hex << std::setw(1) << static_cast<int>(len);

  // 控制位 + 标识位 + 数据
  cmd << control_type << identifier << data;

  // 计算校验和
  uint8_t crc = calculateChecksum(cmd.str());
  cmd << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(crc);

  return cmd.str();
}

/**
 * @brief 构建变长命令 #TP 头
 *
 * @param source_addr
 * @param dest_addr
 * @param control_type
 * @param identifier
 * @param data
 * @return std::string
 */
std::string GimbalCtrl::buildDynamicCommand(const std::string &source_addr,
                                            const std::string &dest_addr,
                                            char control_type,
                                            const std::string &identifier,
                                            const std::vector<uint8_t> &data) {
  //    const std::vector<uint8_t> &data) {

  //   std::vector<uint8_t> data_buf = data; // 拷贝原始数据
  //   if (data_buf.size() < 1) {
  //     data_buf.assign(1, 0); // 不足 2 字节时填充 0
  //   }

  std::ostringstream cmd;

  cmd << "#TP" << source_addr << dest_addr;

  uint8_t len = data.size() * 2; // 字符数*2
  cmd << std::hex << std::uppercase << std::setw(1) << std::setfill('0')
      << static_cast<int>(len); // 仅保留一位字符，0x00-0x0F

  cmd << control_type << identifier;

  for (const auto &byte : data) {
    cmd << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
  }

  std::string cmd_str = cmd.str();
  uint8_t crc = calculateChecksum(cmd_str);
  cmd << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
      << static_cast<int>(crc);

  return cmd.str();
}

/**
 * @brief 构建定长命令
 *
 * @param source_addr
 * @param dest_addr
 * @param control_type
 * @param identifier
 * @param data
 * @return std::string
 */
std::string GimbalCtrl::buildStaticCommand(const std::string &source_addr,
                                           const std::string &dest_addr,
                                           char control_type,
                                           const std::string &identifier,
                                           const uint8_t &data) {
  //    const std::vector<uint8_t> &data) {

  //   std::vector<uint8_t> data_buf = data; // 拷贝原始数据
  //   if (data_buf.size() < 1) {
  //     data_buf.assign(1, 0); // 不足 2 字节时填充 0
  //   }

  std::ostringstream cmd;

  cmd << "#TP" << source_addr << dest_addr;

  uint8_t len = static_cast<uint8_t>(2);
  cmd << std::hex << std::uppercase << std::setw(1) << std::setfill('0')
      << static_cast<int>(len); // 仅保留一位字符，0x00-0x0F

  cmd << control_type << identifier;

  //   for (const auto &byte : data_buf) {
  //     cmd << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
  //         << static_cast<int>(byte);
  //   }

  cmd << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
      << static_cast<int>(data);

  std::string cmd_str = cmd.str();
  uint8_t crc = calculateChecksum(cmd_str);
  cmd << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
      << static_cast<int>(crc);

  return cmd.str();
}

uint8_t GimbalCtrl::calculateChecksum(const std::string &frame) {
  uint8_t crc = 0;
  for (char c : frame) {
    crc += static_cast<uint8_t>(c);
  }
  return crc;
}

std::string GimbalCtrl::hexEncode(int32_t value, int num_digits) {
  std::ostringstream oss;
  oss << std::hex << std::setw(num_digits) << std::setfill('0') << value;
  return oss.str();
}

bool GimbalCtrl::sendAndVerify(const std::string &command) {
  std::lock_guard<std::mutex> lock(socket_mutex_);

  try {
    // 发送命令
    sock_.sendTo(command.data(), command.size(), target_ip_, port_);

    // 接收响应
    const int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    std::string sourceAddr;
    unsigned short sourcePort;

    int received = sock_.recvFrom(buffer, BUFFER_SIZE, sourceAddr, sourcePort);
    if (received <= 0)
      return false;

    std::string response(buffer, received);

    LOG_F(INFO, "Received response: %s", response.c_str());

    // 验证响应
    if (response.find("ERE!!") != std::string::npos) {
      return false;
    }

    // 检查地址交换
    if (response.length() < 5)
      return false;
    if (response.substr(3, 2) != command.substr(5, 2) ||
        response.substr(5, 2) != command.substr(3, 2)) {
      return false;
    }

    return true;

  } catch (SocketException &e) {
    if (error_callback_) {
      error_callback_(e.what());
    }
    return false;
  }
}

bool GimbalCtrl::send(const std::string &command, int timeout_ms) {
  std::lock_guard<std::mutex> lock(socket_mutex_);

  try {
    // 发送命令
    sock_.cleanUp();
    sock_.sendTo(command.data(), command.size(), target_ip_, port_);
    LOG_F(INFO, "Send command: %s", command.c_str());

    // 接收响应
    const int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    std::string sourceAddr;
    unsigned short sourcePort;

    if (timeout_ms <= 0) {
      return true;
    }

    // TODO 超时检测、校验
    // int received = sock_.recvFrom(buffer, BUFFER_SIZE, sourceAddr,
    // sourcePort);
    int received = sock_.recvFromWithTimeout(buffer, sizeof(buffer), sourceAddr,
                                             sourcePort, timeout_ms);
    if (received <= 0) {
      LOG_F(ERROR, "Receive failed, timeout or error");
      return false;
    }

    std::string response(buffer, received);

    LOG_F(INFO, "Received response: %s", response.c_str());

    return true;

  } catch (SocketException &e) {
    if (error_callback_) {
      error_callback_(e.what());
    }
    return false;
  }
}

// bool GimbalCtrl::send(const std::string &command, std::string &response,
//                       int timeout_ms) {
//   std::lock_guard<std::mutex> lock(socket_mutex_);

//   try {
//     // 发送命令
//     sock_.cleanUp();
//     sock_.sendTo(command.data(), command.size(), target_ip_, port_);
//     LOG_F(INFO, "Send command: %s", command.c_str());

//     // 接收响应
//     const int BUFFER_SIZE = 256;
//     char buffer[BUFFER_SIZE];
//     std::string sourceAddr;
//     unsigned short sourcePort;

//     if (timeout_ms <= 0) {
//       return true;
//     }

//     // TODO 超时检测、校验
//     int received = sock_.recvFrom(buffer, BUFFER_SIZE, sourceAddr,
//     sourcePort); if (received <= 0)
//       return false;

//     std::string _response(buffer, received);
//     response = _response;
//     LOG_F(INFO, "Received response: %s", response.c_str());

//     return true;

//   } catch (SocketException &e) {
//     if (error_callback_) {
//       error_callback_(e.what());
//     }
//     return false;
//   }
// }

bool GimbalCtrl::send(const std::string &command, std::string &response,
                      int timeout_ms) {
  std::lock_guard<std::mutex> lock(socket_mutex_);

  try {
    // 发送命令
    sock_.sendTo(command.data(), command.size(), target_ip_, port_);

    if (timeout_ms <= 0)
      return true;

    // 接收响应
    char buffer[256];
    std::string sourceAddr;
    unsigned short sourcePort;

    int received = sock_.recvFromWithTimeout(buffer, sizeof(buffer), sourceAddr,
                                             sourcePort, timeout_ms);

    if (received <= 0) {
      LOG_F(ERROR, "Receive failed, timeout or error");
      return false;
    }

    response.assign(buffer, received);
    return true;

  } catch (SocketException &e) {
    LOG_F(ERROR, "Socket error: %s", e.what());
    return false;
  }
}

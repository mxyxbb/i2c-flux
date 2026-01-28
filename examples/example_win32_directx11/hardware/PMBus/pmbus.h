//pmbus.h - PMBus C++ 封装类头文件
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "../SMBus/smbus.h"  // 引入你提供的 SMBus C API 头文件

// 定义默认配置参数
constexpr uint32_t DEFAULT_BITRATE = 100000;  // 100kHz
constexpr uint8_t  DEFAULT_SLAVE_ADDR = 0x02; // 8-bit
constexpr bool   DEFAULT_AUTO_READ_RESPOND = false;
constexpr uint16_t DEFAULT_WRITE_TIMEOUT = 10;
constexpr uint16_t DEFAULT_READ_TIMEOUT = 10;
constexpr bool   DEFAULT_SCL_LOW_TIMEOUT = true;
constexpr uint16_t DEFAULT_TRANSFER_RETRIES = 1;
constexpr uint32_t DEFAULT_RESPONSE_TIMEOUT = 100;

// INT Return value >=0 means good
// INT Return value < 0 means error
constexpr INT SLAVE_NOT_RESPONSE = -1;
constexpr INT DEVICE_NOT_CONNECTED = -2;
// bool Return true means good, false means error

class PMBus {
public:
    PMBus();
    ~PMBus();

    // 打开设备（传出设备名称，如 assd123456w）
    bool Open(char** deviceName);

    // 关闭设备
    void Close();

    // 配置通信参数（简化版，只允许修改 bitrate，其他用默认值）
    bool Configure(uint32_t bitrate = DEFAULT_BITRATE);

    // 写入数据（类似 I2C 写入寄存器值）
    INT Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data);

    // 读取数据（从指定寄存器读取若干字节）
    INT Read(uint8_t slaveAddress, uint8_t regAddr, uint16_t numBytes, std::vector<uint8_t>& result);

    // 发送一个字节命令码（典型 PMBus 操作）
    INT SendByte(uint8_t slaveAddress, uint8_t byte);

    // 扫描总线上的设备地址
    INT ScanDevices(uint8_t startAddr, uint8_t endAddr, std::vector<uint8_t>& foundAddresses);

    // 获取最后一次错误信息（可扩展）
    std::string GetLastError() const;

private:
    HID_SMBUS_DEVICE device_;  // SMBus C API 的设备句柄
    bool isOpen_{ false };
    std::string lastError_;
};

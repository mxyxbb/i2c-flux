// pmbus.h - PMBus C++ 封装类头文件
#pragma once

#include <cstdint>
#include <vector>
#include <string>

// 引入原有的 SMBus C API 头文件
#include "../SMBus/smbus.h"  

// 引入串口通信和 RPI2C 协议栈头文件
#include "core/serial/include/serial/serial.h" 
#include "../SMBus/RPI2C/RPI2C.h"

// 定义默认配置参数
constexpr uint32_t DEFAULT_BITRATE = 100000;  // 100kHz
constexpr uint8_t  DEFAULT_SLAVE_ADDR = 0x02; // 8-bit
constexpr bool     DEFAULT_AUTO_READ_RESPOND = false;
constexpr uint16_t DEFAULT_WRITE_TIMEOUT = 10;
constexpr uint16_t DEFAULT_READ_TIMEOUT = 10;
constexpr bool     DEFAULT_SCL_LOW_TIMEOUT = true;
constexpr uint16_t DEFAULT_TRANSFER_RETRIES = 1;
constexpr uint32_t DEFAULT_RESPONSE_TIMEOUT = 100;

constexpr INT SLAVE_NOT_RESPONSE = -1;
constexpr INT DEVICE_NOT_CONNECTED = -2;
constexpr INT CRC_ERROR_CODE = -3;

class PMBus {
public:
    PMBus();
    ~PMBus();

    bool Open(char** deviceName);
    void Close();
    bool Configure(uint32_t bitrate = DEFAULT_BITRATE);

    INT Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data);
    INT Read(uint8_t slaveAddress, uint8_t regAddr, uint16_t numBytes, std::vector<uint8_t>& result);
    INT SendByte(uint8_t slaveAddress, uint8_t byte);
    INT ScanDevices(uint8_t startAddr, uint8_t endAddr, std::vector<uint8_t>& foundAddresses);

    std::string GetLastError() const;

    // --- 新增：Batch 批处理缓冲机制 (仅针对 Serial 模式有效) ---
    void FlushOn();
    void FlushOff();
    bool Flush();

    void SetCrcConfig(bool enabled, int type) {
        crcEnabled_ = enabled;
        crcType_ = type;
    }
    bool IsCrcEnabled() const { return crcEnabled_; }
    int GetCrcType() const { return crcType_; }

private:
    enum class DeviceMode {
        MODE_NONE,
        MODE_SMBUS,
        MODE_SERIAL
    };

    DeviceMode currentMode_{ DeviceMode::MODE_NONE };
    std::string lastError_;

    // SMBus 模式相关
    HID_SMBUS_DEVICE device_;

    // Serial 模式相关
    serial::Serial serialPort_;
    RPI2C::Protocol protocolParser_;

    // 内部辅助函数：执行单条串口命令
    bool executeSerialCommand(const std::vector<uint8_t>& tx_data, RPI2C::Packet& rx_packet, int timeout_ms = 100);

    // --- 新增：Flush 机制相关变量 ---
    struct PendingTask {
        enum Type { WRITE, READ } type;
        std::vector<uint8_t>* readResult; // 保存用户引用的指针，用于填入读取数据
        uint16_t readLen;                 // 期望的读取长度
        uint8_t slaveAddress; // 新增：用于记录从机地址
        uint8_t regAddr;      // 新增：用于记录寄存器地址
        bool useCrc;          // 新增：记录入队时是否开启了 CRC
        int crcType;          //【新增】记录入队时的 CRC 算法类型 ---
    };

    bool flushMode_{ false };
    std::vector<uint8_t> txBuffer_;           // 统一发送的指令流
    std::vector<PendingTask> commandQueue_;   // 记录任务顺序以便解包

    bool crcEnabled_ = false;
    int crcType_ = 0;

};

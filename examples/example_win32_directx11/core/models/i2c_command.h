#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace I2CDebugger {

    //// 前向声明
    //struct ParseConfig;

    // 波特率常量
    constexpr uint32_t BAUD_RATE_100K = 100000;
    constexpr uint32_t BAUD_RATE_400K = 400000;

    struct ParseConfig {
        std::string readFormula;   // 读取公式
        std::string writeFormula;  // 写入公式
        bool enabled = false;
    };

    // 错误类型枚举
    enum class ErrorType {
        None = 0,
        SlaveNotResponse,// 从机无响应
        DeviceDisconnected,  // 设备断开
        UnknownError
    };

    // 命令类型枚举
    enum class CommandType {
        Read = 0,
        Write,
        SendCommand
    };

    // 寄存器表项
    struct RegisterEntry {
        uint8_t regAddress = 0x00;
        uint8_t length = 1;
        std::vector<uint8_t> data; std::string description;

        bool overrideSlaveAddr = false;
        uint8_t slaveAddress = 0x50;
        bool lastSuccess = false;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;
    };

    // 单次触发命令项
    struct SingleTriggerEntry {
        bool enabled = true;
        uint8_t regAddress = 0x00;
        uint8_t length = 1;
        std::vector<uint8_t> data;
        uint32_t delayMs = 0;
        CommandType type = CommandType::Read;
        std::string buttonName = "执行";

        bool overrideSlaveAddr = false;
        uint8_t slaveAddress = 0x50;
        bool lastSuccess = false;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;
    };

    // 周期触发命令项
    struct PeriodicTriggerEntry {
        bool enabled = true;
        uint8_t regAddress = 0x00;
        uint8_t length = 1;
        std::vector<uint8_t> data;
        uint32_t delayMs = 0;
        CommandType type = CommandType::Read;
        std::string buttonName = "执行";

        // 从机地址覆写
        bool overrideSlaveAddr = false;
        uint8_t slaveAddress = 0x50;

        // 解析配置
        std::string alias;           // 别名 (用于曲线图例和CSV表头)
        std::string formula;         // 读取公式 (Raw → 十进制)
        std::string writeFormula;    // 写入公式 (十进制 → Raw)
        bool parseConfigured = false; // 是否已配置解析

        // 解析结果
        double parsedValue = 0.0;    // 解析后的十进制值
        bool parseSuccess = false;   // 解析是否成功

        // 曲线显示
        bool showCurve = false;

        ParseConfig parseConfig;

        // 执行状态
        bool lastSuccess = false;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;       // 错误信息 (注意：不是 errorMsg)

        // 错误计数（NAK次数）
        uint32_t errorCount = 0;
    };

    // 命令表组
    struct CommandGroup {
        std::string name = "example group1";
        uint8_t slaveAddress = 0x50;
        uint32_t interval = 100;
        bool continueOnError = false;

        std::vector<RegisterEntry> registerEntries;
        std::vector<SingleTriggerEntry> singleTriggerEntries;
        std::vector<PeriodicTriggerEntry> periodicTriggerEntries;
    };

    // 扫描到的从机信息
    struct SlaveInfo {
        uint8_t address = 0;
        bool selected = false;
    };

    // 响应数据包
    struct ResponsePacket {
        uint32_t controlId = 0;
        uint32_t commandId = 0;
        std::vector<uint8_t> rawData;
        uint64_t timestamp = 0;
        bool success = false;
        ErrorType errorType = ErrorType::None;
        std::string errorMsg;
    };

}

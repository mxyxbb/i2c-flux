#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace I2CDebugger {

    // ========== 波特率常量 ==========
    constexpr uint32_t BAUD_RATE_100K = 100000;
    constexpr uint32_t BAUD_RATE_400K = 400000;

    // ========== 解析配置结构 ==========
    struct ParseConfig {
        bool enabled = false;
        std::string readFormula;      // 读取公式
        std::string writeFormula;     // 写入公式
        std::string alias;            // 别名（用于log表头）

        // 运行时数据（不保存到JSON）
        double parsedValue = 0.0;
        bool parseSuccess = false;
        std::string lastError;
    };

    // ========== 错误类型枚举 ==========
    enum class ErrorType {
        None = 0,
        SlaveNotResponse,    // 从机无响应
        DeviceDisconnected,  // 设备断开
        UnknownError
    };

    // ========== 命令类型枚举 ==========
    enum class CommandType {
        Read = 0,
        Write,
        SendCommand
    };

    // ========== 扫描到的从机信息 ==========
    struct SlaveInfo {
        uint8_t address = 0;
        bool selected = false;
    };

    // ========== 响应数据包 ==========
    struct ResponsePacket {
        uint32_t controlId = 0;
        uint32_t commandId = 0;
        std::vector<uint8_t> rawData;
        uint64_t timestamp = 0;
        bool success = false;
        ErrorType errorType = ErrorType::None;
        std::string errorMsg;
    };

    // 注意：RegisterEntry, SingleTriggerEntry, PeriodicTriggerEntry, CommandGroup
    // 这些结构体定义在 i2c_table_app.h 中

} // namespace I2CDebugger

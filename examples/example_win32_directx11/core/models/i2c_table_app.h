#pragma once

#include "i2c_command.h"
#include "../ui/widgets/activity_indicator.h"
#include <string>
#include <vector>

namespace I2CDebugger {

    // ========== 寄存器表条目 ==========
    struct RegisterEntry {
        uint8_t regAddress = 0x00;
        uint8_t length = 1;
        std::vector<uint8_t> data;
        std::string description;

        bool overrideSlaveAddr = false;
        uint8_t slaveAddress = 0x50;

        bool lastSuccess = true;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;

        // 解析配置
        ParseConfig parseConfig;
    };

    // ========== 单次触发条目 ==========
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

        bool lastSuccess = true;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;

        // 解析配置
        ParseConfig parseConfig;
    };

    // ========== 周期触发条目 ==========
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

        // 执行状态
        bool lastSuccess = true;
        ErrorType lastErrorType = ErrorType::None;
        std::string lastError;
        uint32_t errorCount = 0;

        // 解析配置
        ParseConfig parseConfig;

        // 曲线相关
        bool plotEnabled = false;
    };

    // ========== 数据日志配置（统一定义） ==========
    struct DataLogConfig {
        bool enabled = false;
        std::string filePath;
        bool useAlias = true;           // 使用别名作为表头
        bool includeTimestamp = true;   // 包含时间戳
        bool logRawData = true;         // 记录原始数据
        bool logParsedValue = true;     // 记录解析值
    };

    // ========== 命令组 ==========
    struct CommandGroup {
        std::string name = "New Group";
        uint8_t slaveAddress = 0x50;
        uint32_t interval = 100;
        bool continueOnError = false;

        std::vector<RegisterEntry> registerEntries;
        std::vector<SingleTriggerEntry> singleTriggerEntries;
        std::vector<PeriodicTriggerEntry> periodicTriggerEntries;

        // 数据日志配置
        DataLogConfig logConfig;
    };

    // ========== Tab类型枚举 ==========
    enum class TabType {
        RegisterTable = 0,
        SingleTrigger,
        PeriodicTrigger
    };

    // ========== 表格窗口应用数据 ==========
    struct I2CTableAppData {
        // 连接状态
        bool isConnected = false;
        std::string deviceName;
        uint32_t baudRate = BAUD_RATE_100K;

        // 命令组
        std::vector<CommandGroup> commandGroups;
        int currentGroupIndex = 0;

        // 当前Tab
        TabType currentTab = TabType::RegisterTable;

        // 选中行索引
        int selectedRowRegister = -1;
        int selectedRowSingle = -1;
        int selectedRowPeriodic = -1;

        // 周期执行状态
        bool isPeriodicRunning = false;
        // 寄存器表读取状态
        bool isReadingAllRegisters = false;
        bool isExecuteAllSingleCommands = false;

        // 活动指示灯
        ActivityIndicator activityIndicator;
    };

} // namespace I2CDebugger

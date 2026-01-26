#pragma once

#include "i2c_command.h"
#include "../ui/widgets/activity_indicator.h"
#include <string>
#include <vector>

namespace I2CDebugger {

    enum class TabType {
        RegisterTable = 0,
        SingleTrigger,
        PeriodicTrigger
    };

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

}

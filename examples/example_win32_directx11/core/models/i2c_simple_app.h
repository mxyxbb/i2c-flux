#pragma once

#include "i2c_command.h"
#include "../ui/widgets/activity_indicator.h"
#include <string>
#include <vector>

namespace I2CDebugger {

    enum class OperationType {
        Read = 0,
        Write,
        SendCommand
    };

    struct I2CSimpleAppData {
        // 连接状态
        bool isConnected = false;
        std::string deviceName;
        uint32_t baudRate = BAUD_RATE_100K;

        // 扫描状态
        bool isScanning = false;
        std::vector<SlaveInfo> scannedSlaves;

        // 操作状态
        bool isOperating = false;
        OperationType operationType = OperationType::Read;

        // 输入缓冲区
        char slaveAddrInput[16] = "0x50";
        char regAddrInput[16] = "0x00";
        char lengthInput[16] = "1";
        char writeDataInput[256] = "";

        // 读取结果
        std::vector<uint8_t> readData;

        // 操作结果
        bool lastOperationSuccess = true;
        std::string lastErrorMessage;

        // 活动指示灯
        ActivityIndicator activityIndicator;
    };

}

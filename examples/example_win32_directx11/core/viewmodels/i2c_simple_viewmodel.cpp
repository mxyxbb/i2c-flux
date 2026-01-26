#include "i2c_simple_viewmodel.h"
#include <sstream>
#include <iomanip>

namespace I2CDebugger {

    I2CSimpleViewModel::I2CSimpleViewModel(std::shared_ptr<HardwareService> hardwareService)
        : m_hardwareService(hardwareService)
    {
        //回调统一在App中设置，避免覆盖问题
    }

    void I2CSimpleViewModel::Connect()
    {
        if (m_data.isConnected) {
            Disconnect();
        }
        else {
            m_hardwareService->Connect(m_data.baudRate);
        }
    }

    void I2CSimpleViewModel::Disconnect()
    {
        m_hardwareService->Disconnect();
    }

    void I2CSimpleViewModel::ScanSlaves()
    {
        if (!m_data.isConnected) {
            m_data.lastOperationSuccess = false;
            m_data.lastErrorMessage = "设备未连接";
            return;
        }
        m_data.isScanning = true;
        m_data.lastErrorMessage.clear();
        m_hardwareService->ScanSlaves();
    }

    void I2CSimpleViewModel::ExecuteOperation()
    {
        if (!m_data.isConnected) {
            m_data.lastOperationSuccess = false;
            m_data.lastErrorMessage = "设备未连接";
            return;
        }

        uint8_t slaveAddr = ParseHexInput(m_data.slaveAddrInput);
        uint8_t regAddr = ParseHexInput(m_data.regAddrInput);
        m_data.isOperating = true;
        m_data.lastErrorMessage.clear();

        switch (m_data.operationType) {
        case OperationType::Read: {
            uint8_t length = 1;
            try {
                length = static_cast<uint8_t>(std::stoi(m_data.lengthInput));
            }
            catch (...) {
                length = 1;
            }
            m_hardwareService->ReadRegister(slaveAddr, regAddr, length, 0, 0);
            break;
        }
        case OperationType::Write: {
            auto data = ParseHexDataInput(m_data.writeDataInput);
            m_hardwareService->WriteRegister(slaveAddr, regAddr, data, 0, 1);
            break;
        }
        case OperationType::SendCommand: {
            m_hardwareService->SendCommand(slaveAddr, regAddr, 0, 2);
            break;
        }
        }
    }

    void I2CSimpleViewModel::SelectSlave(int index)
    {
        if (index >= 0 && index < static_cast<int>(m_data.scannedSlaves.size())) {
            for (auto& slave : m_data.scannedSlaves) {
                slave.selected = false;
            }
            m_data.scannedSlaves[index].selected = true;
            std::snprintf(m_data.slaveAddrInput, sizeof(m_data.slaveAddrInput),
                "0x%02X", m_data.scannedSlaves[index].address);
        }
    }

    uint8_t I2CSimpleViewModel::ParseHexInput(const char* input) const
    {
        std::string str(input);
        if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            str = str.substr(2);
        }
        if (str.empty()) return 0;
        try {
            return static_cast<uint8_t>(std::stoul(str, nullptr, 16));
        }
        catch (...) {
            return 0;
        }
    }

    std::vector<uint8_t> I2CSimpleViewModel::ParseHexDataInput(const char* input) const
    {
        std::vector<uint8_t> result;
        std::istringstream iss(input);
        std::string token;

        while (iss >> token) {
            if (token.size() >= 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
                token = token.substr(2);
            }
            if (!token.empty()) {
                try {
                    result.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
                }
                catch (...) {
                    // 忽略解析错误
                }
            }
        }

        return result;
    }

    std::string I2CSimpleViewModel::FormatHexData(const std::vector<uint8_t>& data) const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < data.size(); i++) {
            if (i > 0) oss << " ";
            oss << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

}

#include "crc_viewmodel.h"
#include "../models/crc_calculator.h"
#include "i2c_table_viewmodel.h"
#include <cstdio>

namespace I2CDebugger {

    CrcViewModel::CrcViewModel(std::shared_ptr<I2CTableViewModel> mainViewModel)
        : m_mainViewModel(mainViewModel) {
    }

    int CrcViewModel::GetCrcType() const {
        return m_mainViewModel->GetCurrentGroup().crcType;
    }

    void CrcViewModel::SetCrcType(int type) {
        m_mainViewModel->GetCurrentGroup1().crcType = type;
    }

    void CrcViewModel::ExecuteCalculation(const std::string& hexInput) {
        // 1. 调用主 ViewModel 现有的解析方法将 Hex 字符串转为字节数组
        std::vector<uint8_t> data = m_mainViewModel->ParseHexDataInput(hexInput.c_str());

        if (data.empty()) {
            m_testResult = "无效输入";
            return;
        }

        // 2. 调用 Model 层进行计算
        int type = GetCrcType();
        uint32_t crcValue = CrcCalculator::Calculate(type, data);

        // 3. 格式化结果并更新状态
        char buf[64];
        if (type == 0) std::snprintf(buf, sizeof(buf), "0x%02X", crcValue);
        else if (type == 1) std::snprintf(buf, sizeof(buf), "0x%04X", crcValue);
        else std::snprintf(buf, sizeof(buf), "0x%08X", crcValue);

        m_testResult = buf;
    }

    void CrcViewModel::ExecuteSimCalculation(const std::string& slaveAddr, int rwMode,
        const std::string& regAddr, const std::string& lenStr,
        const std::string& data)
    {
        // 1. 解析输入值
        uint8_t addr = m_mainViewModel->ParseHexInput(slaveAddr.c_str());
        uint8_t reg = m_mainViewModel->ParseHexInput(regAddr.c_str());

        int len = 0;
        try { len = std::stoi(lenStr); }
        catch (...) {}
        if (len < 0) len = 0;

        // 2. 解析数据并根据长度截断/补零
        std::vector<uint8_t> dataBytes = m_mainViewModel->ParseHexDataInput(data.c_str());
        dataBytes.resize(len, 0); // 确保长度与用户输入的 length 字段一致

        // 3. 构建底层需要的基础字节
        uint8_t addrW = (addr << 1) | 0x00; // 设备的写地址
        uint8_t addrR = (addr << 1) | 0x01; // 设备的读地址
        uint8_t byte2 = reg;                // 寄存器地址

        // 4. 更新供 View 显示的文本状态 (用于界面呈现)
        char buf[32];
        uint8_t displayByte1 = (addr << 1) | (rwMode & 0x01);
        std::snprintf(buf, sizeof(buf), "0x%02X", displayByte1 & 0xfe);
        m_simByte1Str_w = buf;
        std::snprintf(buf, sizeof(buf), "0x%02X", displayByte1 | 0x1);
        m_simByte1Str_r = buf;

        std::snprintf(buf, sizeof(buf), "0x%02X", byte2);
        m_simByte2Str = buf;

        m_simDataStr = m_mainViewModel->FormatHexData(dataBytes);
        if (m_simDataStr.empty()) m_simDataStr = "(无数据)";

        // 5. 将所有字节拼接到一起进行 CRC 计算
        std::vector<uint8_t> crcPayload;

        if (rwMode == 1) {
            // 读取(Read)的拼接方法: 包含写地址(发寄存器)、寄存器地址、读地址(读数据方向)、读取到的数据
            crcPayload.push_back(addrW);
            crcPayload.push_back(byte2);
            crcPayload.push_back(addrR);
        }
        else {
            // 写入(Write)的拼接方法: 包含写地址、寄存器地址、写入的数据
            crcPayload.push_back(addrW);
            crcPayload.push_back(byte2);
        }

        // 统一在尾部追加数据
        crcPayload.insert(crcPayload.end(), dataBytes.begin(), dataBytes.end());

        // 6. 调用底层 Model 算法进行计算
        int type = GetCrcType();
        uint32_t crcValue = CrcCalculator::Calculate(type, crcPayload);

        // 7. 格式化结果
        if (type == 0) std::snprintf(buf, sizeof(buf), "0x%02X", crcValue);
        else if (type == 1) std::snprintf(buf, sizeof(buf), "0x%04X", crcValue);
        else std::snprintf(buf, sizeof(buf), "0x%08X", crcValue);

        m_simCrcResult = buf;
    }

    void CrcViewModel::Reset() {
        m_testResult = "---";
        m_simByte1Str_w = "--";
        m_simByte1Str_r = "--";
        m_simByte2Str = "--";
        m_simDataStr = "--";
        m_simCrcResult = "---";
    }
}

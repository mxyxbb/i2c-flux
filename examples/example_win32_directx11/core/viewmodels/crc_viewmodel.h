#pragma once
#include <string>
#include <memory>
#include <vector>

namespace I2CDebugger {

    class I2CTableViewModel; // 前向声明

    class CrcViewModel {
    public:
        explicit CrcViewModel(std::shared_ptr<I2CTableViewModel> mainViewModel);

        // 数据绑定：获取和设置当前的 CRC 类型
        int GetCrcType() const;
        void SetCrcType(int type);

        // 数据绑定：获取计算结果文本
        const std::string& GetTestResult() const { return m_testResult; }

        // 命令：执行计算
        void ExecuteCalculation(const std::string& hexInput);

        // 命令：重置状态
        void Reset();
        // 命令：执行模拟读写 CRC 计算
        void ExecuteSimCalculation(const std::string& slaveAddr, int rwMode,
            const std::string& regAddr, const std::string& lenStr,
            const std::string& data);

        // 数据绑定：获取模拟计算的中间过程和结果
        const std::string& GetSimByte1Str() const { return m_simByte1Str; }
        const std::string& GetSimByte2Str() const { return m_simByte2Str; }
        const std::string& GetSimDataStr() const { return m_simDataStr; }
        const std::string& GetSimCrcResult() const { return m_simCrcResult; }

    private:
        std::shared_ptr<I2CTableViewModel> m_mainViewModel;
        std::string m_testResult = "---";

        // --- 新增：用于 UI 绑定的只读字符串状态 ---
        std::string m_simByte1Str = "--";
        std::string m_simByte2Str = "--";
        std::string m_simDataStr = "--";
        std::string m_simCrcResult = "---";
    };
}

#pragma once

#include "../models/i2c_table_app.h"
#include "../services/hardware_service.h"
#include "../services/configuration_service.h"
#include "../services/expression_parser.h"
#include "../services/data_logger.h"
#include "plot_viewmodel.h"
#include <memory>
#include <string>

namespace I2CDebugger {

    class ConfigurationService;
    class DataLogger;  // 前向声明

    // 1. 定义撤销类型枚举
    enum class UndoItemType {
        Register,
        SingleTrigger,
        PeriodicTrigger
    };

    // 2. 定义撤销动作结构体
    struct UndoAction {
        UndoItemType type;
        int groupIndex;
        int itemIndex;

        // 为了兼容性，这里简单地把三种 entry 都放进来
        // 实际存储时只会用到其中一个
        RegisterEntry regEntry;
        SingleTriggerEntry singleEntry;
        PeriodicTriggerEntry periodicEntry;
    };

    class I2CTableViewModel {
    public:
        explicit I2CTableViewModel(std::shared_ptr<HardwareService> hardwareService,
            std::shared_ptr<PlotViewModel> plotViewModel);
        ~I2CTableViewModel();

        void Connect();
        void Disconnect();

        void AddGroup();
        void RenameGroup(const std::string& newName);
        void DeleteGroup();
        // 拖动排序：交换两个命令表的位置，并同步当前选中索引
        void MoveGroup(int fromIndex, int toIndex);

        // 导出/导入单个命令表
        bool ExportGroup(const std::string& filePath);
        bool ImportGroup(const std::string& filePath);

        void AddRegisterEntry();
        void DeleteRegisterEntry();
        void CopyRegisterEntry();
        void MoveRegisterEntryUp();
        void MoveRegisterEntryDown();
        void ReadAllRegisters();

        void AddSingleEntry();
        void DeleteSingleEntry();
        void CopySingleEntry();
        void MoveSingleEntryUp();
        void MoveSingleEntryDown();
        void ExecuteSingleCommand(int index);
        void ExecuteAllSingleCommands();

        void SetAllSingleEntriesEnabled(bool enabled);
        bool AreAllSingleEntriesEnabled() const;
        bool AreAnySingleEntriesEnabled() const;

        void AddPeriodicEntry();
        void DeletePeriodicEntry();
        void CopyPeriodicEntry();
        void MovePeriodicEntryUp();
        void MovePeriodicEntryDown();
        void ExecutePeriodicCommand(int index);
        void StartPeriodicExecution();
        void StopPeriodicExecution();

        void SetAllPeriodicEntriesEnabled(bool enabled);
        bool AreAllPeriodicEntriesEnabled() const;
        bool AreAnyPeriodicEntriesEnabled() const;

        void ResetPeriodicErrorCounts();

        CommandGroup& GetCurrentGroup1();
        const CommandGroup& GetCurrentGroup() const;  // 添加 const 版本

        I2CTableAppData& GetData() { return m_data; }
        const I2CTableAppData& GetData() const { return m_data; }

        uint8_t ParseHexInput(const char* input) const;
        std::string FormatHexData(const std::vector<uint8_t>& data) const;
        std::vector<uint8_t> ParseHexDataInput(const char* input) const;
        void OnDataResult(const ResponsePacket& packet);
        // ============== 解析相关方法（扩展） ==============

        // 寄存器表解析
        void UpdateRegisterParsedValue(size_t entryIndex);
        ParseConfig& GetRegisterParseConfig(size_t entryIndex);
        void SetRegisterParseConfig(size_t entryIndex, const ParseConfig& config);

        // 单次触发解析
        void UpdateSingleParsedValue(size_t entryIndex);
        void UpdateSingleRawFromParsedValue(size_t entryIndex, const std::string& strValue);
        void UpdateSingleRawFromParsedValue(size_t entryIndex, double newValue);
        ParseConfig& GetSingleParseConfig(size_t entryIndex);
        void SetSingleParseConfig(size_t entryIndex, const ParseConfig& config);

        // 周期触发解析（已有，保持不变）
        void UpdateParsedValue(size_t entryIndex);// 使用读取公式计算解析值
        void UpdateRawFromParsedValue(size_t entryIndex, double newValue);// 使用写入公式从十进制值计算原始字节
        void UpdateRawFromParsedValue(size_t entryIndex, const std::string& strValue);// 从字符串值计算原始字节
        ParseConfig& GetParseConfig(size_t entryIndex);
        void SetParseConfig(size_t entryIndex, const ParseConfig& config);

        std::string GetFormulaHelp() const;

        // ============== 数据日志相关方法 ==============
        DataLogConfig& GetLogConfig() { return m_logConfig; }
        bool IsDataLoggingActive() const { return m_dataLogger->IsActive(); }
        uint32_t GetLoggedDataCount() const { return m_dataLogger->GetLoggedCount(); }

        bool StartDataLogging(const std::string& filePath);
        void StopDataLogging();

        // 暴露给 UI 调用的撤销方法
        void UndoLastDelete();
        // 【新增】：导出单次触发数据到 CSV
        bool ExportSingleTriggerToCSV(const std::string& filePath);

    private:
        I2CTableAppData m_data;
        std::shared_ptr<HardwareService> m_hardwareService;
        std::shared_ptr<ConfigurationService> m_configService;
        std::unique_ptr<ExpressionParser> m_expressionParser;  // 添加
        std::unique_ptr<DataLogger> m_dataLogger;
        DataLogConfig m_logConfig;
        std::shared_ptr<PlotViewModel> m_plotViewModel; // 新增成员变量

        // 添加撤销栈（限制大小防止内存无限增长）
        std::vector<UndoAction> m_undoStack;
        const size_t MAX_UNDO_STEPS = 20;
    };

}

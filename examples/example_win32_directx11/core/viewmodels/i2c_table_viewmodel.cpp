#include "../models/i2c_table_app.h"
#include "i2c_table_viewmodel.h"
#include "../services/data_logger.h"
#include "../services/expression_parser.h"  // 新增：引入表达式解析器
#include <fstream>
#include <sstream>
#include <iomanip>

namespace I2CDebugger {

    I2CTableViewModel::I2CTableViewModel(std::shared_ptr<HardwareService> hardwareService,
        std::shared_ptr<PlotViewModel> plotViewModel)
        : m_hardwareService(hardwareService)
        , m_configService(std::make_shared<ConfigurationService>()) // 修复：之前未初始化，导出/导入失败路径写入 m_lastError 时会通过空指针崩溃
        , m_plotViewModel(plotViewModel) // 初始化
        , m_expressionParser(std::make_unique<ExpressionParser>())
        , m_dataLogger(std::make_unique<DataLogger>())  // 新增
    {
        // 初始化默认命令组
        CommandGroup group1;
        group1.name = "example group1";
        group1.slaveAddress = 0x50;
        RegisterEntry regEntry;
        regEntry.regAddress = 0x00;
        regEntry.length = 1;
        group1.registerEntries.push_back(regEntry);

        SingleTriggerEntry singleEntry;
        singleEntry.regAddress = 0x00;
        singleEntry.length = 1;
        group1.singleTriggerEntries.push_back(singleEntry);

        PeriodicTriggerEntry periodicEntry;
        periodicEntry.regAddress = 0x00;
        periodicEntry.length = 1;
        group1.periodicTriggerEntries.push_back(periodicEntry);

        m_data.commandGroups.push_back(group1);

        CommandGroup group2;
        group2.name = "example group 2";
        group2.slaveAddress = 0x51;
        m_data.commandGroups.push_back(group2);
    }

    I2CTableViewModel::~I2CTableViewModel() {
        // 确保停止日志记录
        if (m_dataLogger && m_dataLogger->IsActive()) {
            m_dataLogger->Stop();
        }
    }

    // ============== 数据日志方法实现 ==============

    bool I2CTableViewModel::StartDataLogging(const std::string& filePath) {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        m_logConfig.filePath = filePath;
        return m_dataLogger->Start(filePath, entries, m_logConfig);
    }

    void I2CTableViewModel::StopDataLogging() {
        m_dataLogger->Stop();
    }

    void I2CTableViewModel::Connect()
    {
        if (m_data.isConnected) {
            Disconnect();
        }
        else {
            m_hardwareService->Connect(m_data.baudRate);
        }
    }

    void I2CTableViewModel::Disconnect()
    {
        m_hardwareService->Disconnect();
    }

    CommandGroup& I2CTableViewModel::GetCurrentGroup1()
    {
        if (m_data.currentGroupIndex < 0 ||
            m_data.currentGroupIndex >= static_cast<int>(m_data.commandGroups.size())) {
            m_data.currentGroupIndex = 0;
        }
        return m_data.commandGroups[m_data.currentGroupIndex];
    }

    const CommandGroup& I2CTableViewModel::GetCurrentGroup() const
    {
        if (m_data.currentGroupIndex < 0 ||
            m_data.currentGroupIndex >= static_cast<int>(m_data.commandGroups.size())) {
            return m_data.commandGroups[0];
        }
        return m_data.commandGroups[m_data.currentGroupIndex];
    }

    void I2CTableViewModel::AddGroup()
    {
        CommandGroup newGroup;
        newGroup.name = "New Group " + std::to_string(m_data.commandGroups.size() + 1);
        m_data.commandGroups.push_back(newGroup);
        m_data.currentGroupIndex = static_cast<int>(m_data.commandGroups.size()) - 1;
    }

    void I2CTableViewModel::RenameGroup(const std::string& newName)
    {
        if (!newName.empty()) {
            GetCurrentGroup1().name = newName;
        }
    }

    void I2CTableViewModel::DeleteGroup()
    {
        if (m_data.commandGroups.size() > 1) {
            m_data.commandGroups.erase(m_data.commandGroups.begin() + m_data.currentGroupIndex);
            if (m_data.currentGroupIndex >= static_cast<int>(m_data.commandGroups.size())) {
                m_data.currentGroupIndex = static_cast<int>(m_data.commandGroups.size()) - 1;
            }
        }
    }

    // 交换两个命令表的位置（用于下拉列表的拖动排序）
    void I2CTableViewModel::MoveGroup(int fromIndex, int toIndex)
    {
        int count = static_cast<int>(m_data.commandGroups.size());
        if (fromIndex < 0 || fromIndex >= count ||
            toIndex < 0 || toIndex >= count || fromIndex == toIndex) {
            return;
        }

        std::swap(m_data.commandGroups[fromIndex], m_data.commandGroups[toIndex]);

        // 让当前选中索引跟随被移动的命令表，保证显示的命令表不变
        if (m_data.currentGroupIndex == fromIndex) {
            m_data.currentGroupIndex = toIndex;
        }
        else if (m_data.currentGroupIndex == toIndex) {
            m_data.currentGroupIndex = fromIndex;
        }
    }

    //导出当前命令表
    bool I2CTableViewModel::ExportGroup(const std::string& filePath) {
        return m_configService->ExportCommandGroup(m_data, m_data.currentGroupIndex, filePath);
    }

    // 导入命令表（作为新的命令组添加）
    bool I2CTableViewModel::ImportGroup(const std::string& filePath) {
        return m_configService->ImportCommandGroup(m_data, filePath, true);
    }

    //寄存器表操作
    void I2CTableViewModel::AddRegisterEntry()
    {
        RegisterEntry entry;
        entry.regAddress = 0x00;
        entry.length = 1;
        GetCurrentGroup1().registerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeleteRegisterEntry() {
        auto& data = GetData();
        auto& entries = GetCurrentGroup1().registerEntries;

        if (data.selectedRowRegister >= 0 && data.selectedRowRegister < static_cast<int>(entries.size())) {

            // 【新增】：在真正 erase 之前，保存到撤销栈
            UndoAction action;
            action.type = UndoItemType::Register;
            action.groupIndex = data.currentGroupIndex;
            action.itemIndex = data.selectedRowRegister;
            action.regEntry = entries[data.selectedRowRegister];

            m_undoStack.push_back(action);
            if (m_undoStack.size() > MAX_UNDO_STEPS) {
                m_undoStack.erase(m_undoStack.begin()); // 保持栈大小不超过限制
            }

            // 原有的删除逻辑
            entries.erase(entries.begin() + data.selectedRowRegister);

            // 修正越界的选中行索引
            if (data.selectedRowRegister >= static_cast<int>(entries.size())) {
                data.selectedRowRegister = static_cast<int>(entries.size()) - 1;
            }
        }
    }

    void I2CTableViewModel::CopyRegisterEntry()
    {
        auto& entries = GetCurrentGroup1().registerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowRegister;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }

        RegisterEntry copy = entries[index];
        entries.insert(entries.begin() + index + 1, copy);
        m_data.selectedRowRegister = index + 1;
    }

    void I2CTableViewModel::MoveRegisterEntryUp()
    {
        auto& entries = GetCurrentGroup1().registerEntries;
        int index = m_data.selectedRowRegister;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowRegister--;
        }
    }

    void I2CTableViewModel::MoveRegisterEntryDown()
    {
        auto& entries = GetCurrentGroup1().registerEntries;
        int index = m_data.selectedRowRegister;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowRegister++;
        }
    }

    void I2CTableViewModel::ReadAllRegisters()
    {
        if (!m_data.isConnected || m_data.isReadingAllRegisters) {
            return;
        }
        auto& group = GetCurrentGroup1();
        if (group.registerEntries.empty()) {
            return;
        }
        m_data.isReadingAllRegisters = true;
        m_hardwareService->EnableBatchMode(true);
        // 【修改】传入当前组的 CRC 配置
        m_hardwareService->ReadAllRegisters(group.slaveAddress, group.registerEntries, group.crcEnabled, group.crcType);
    }

    // 单次触发操作
    void I2CTableViewModel::AddSingleEntry()
    {
        SingleTriggerEntry entry;
        entry.regAddress = 0x00;
        entry.length = 1;
        GetCurrentGroup1().singleTriggerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeleteSingleEntry() {
        auto& data = GetData();
        auto& entries = GetCurrentGroup1().singleTriggerEntries;

        if (data.selectedRowSingle >= 0 && data.selectedRowSingle < static_cast<int>(entries.size())) {

            UndoAction action;
            action.type = UndoItemType::SingleTrigger;
            action.groupIndex = data.currentGroupIndex;
            action.itemIndex = data.selectedRowSingle;
            action.singleEntry = entries[data.selectedRowSingle];

            m_undoStack.push_back(action);
            if (m_undoStack.size() > MAX_UNDO_STEPS) {
                m_undoStack.erase(m_undoStack.begin());
            }

            entries.erase(entries.begin() + data.selectedRowSingle);

            if (data.selectedRowSingle >= static_cast<int>(entries.size())) {
                data.selectedRowSingle = static_cast<int>(entries.size()) - 1;
            }
        }
    }

    void I2CTableViewModel::CopySingleEntry()
    {
        auto& entries = GetCurrentGroup1().singleTriggerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowSingle;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }

        SingleTriggerEntry copy = entries[index];
        entries.insert(entries.begin() + index + 1, copy);
        m_data.selectedRowSingle = index + 1;
    }

    void I2CTableViewModel::MoveSingleEntryUp()
    {
        auto& entries = GetCurrentGroup1().singleTriggerEntries;
        int index = m_data.selectedRowSingle;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowSingle--;
        }
    }

    void I2CTableViewModel::MoveSingleEntryDown()
    {
        auto& entries = GetCurrentGroup1().singleTriggerEntries;
        int index = m_data.selectedRowSingle;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowSingle++;
        }
    }

    void I2CTableViewModel::ExecuteSingleCommand(int index)
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup1();
        if (index < 0 || index >= static_cast<int>(group.singleTriggerEntries.size())) return;

        const auto& entry = group.singleTriggerEntries[index];
        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : group.slaveAddress;

        // 【修改】追加 CRC 参数
        switch (entry.type) {
        case CommandType::Read:
            m_hardwareService->InsertSingleRead(slaveAddr, entry.regAddress, entry.length, 2, index, group.crcEnabled, group.crcType);
            break;
        case CommandType::Write:
            m_hardwareService->InsertSingleWrite(slaveAddr, entry.regAddress, entry.data, 2, index, group.crcEnabled, group.crcType);
            break;
        case CommandType::SendCommand:
            m_hardwareService->InsertSingleCommand(slaveAddr, entry.regAddress, 2, index, group.crcEnabled, group.crcType);
            break;
        }
    }

    void I2CTableViewModel::ExecuteAllSingleCommands()
    {
        if (!m_data.isConnected || m_data.isExecuteAllSingleCommands) {
            return;
        }
        auto& group = GetCurrentGroup1();
        if (group.singleTriggerEntries.empty()) {
            return;
        }
        if (!AreAnySingleEntriesEnabled()) {
            return;
        }
        m_data.isExecuteAllSingleCommands = true;
        m_hardwareService->EnableBatchMode(true);
        // 【修改】追加 CRC 参数
        m_hardwareService->ExecuteAllSingleTrigger(group.slaveAddress, group.singleTriggerEntries, group.crcEnabled, group.crcType);
    }

    void I2CTableViewModel::SetAllSingleEntriesEnabled(bool enabled)
    {
        auto& entries = GetCurrentGroup1().singleTriggerEntries;
        for (auto& entry : entries) {
            entry.enabled = enabled;
        }
    }

    bool I2CTableViewModel::AreAllSingleEntriesEnabled() const
    {
        const auto& entries = GetCurrentGroup().singleTriggerEntries;
        if (entries.empty()) return false;
        for (const auto& entry : entries) {
            if (!entry.enabled) return false;
        }
        return true;
    }

    bool I2CTableViewModel::AreAnySingleEntriesEnabled() const
    {
        const auto& entries = GetCurrentGroup().singleTriggerEntries;
        for (const auto& entry : entries) {
            if (entry.enabled) return true;
        }
        return false;
    }

    // 周期触发操作
    void I2CTableViewModel::AddPeriodicEntry()
    {
        PeriodicTriggerEntry entry;
        entry.regAddress = 0x00;
        entry.length = 1;
        GetCurrentGroup1().periodicTriggerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeletePeriodicEntry() {
        auto& data = GetData();
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;

        if (data.selectedRowPeriodic >= 0 && data.selectedRowPeriodic < static_cast<int>(entries.size())) {

            UndoAction action;
            action.type = UndoItemType::PeriodicTrigger;
            action.groupIndex = data.currentGroupIndex;
            action.itemIndex = data.selectedRowPeriodic;
            action.periodicEntry = entries[data.selectedRowPeriodic];

            m_undoStack.push_back(action);
            if (m_undoStack.size() > MAX_UNDO_STEPS) {
                m_undoStack.erase(m_undoStack.begin());
            }

            entries.erase(entries.begin() + data.selectedRowPeriodic);

            if (data.selectedRowPeriodic >= static_cast<int>(entries.size())) {
                data.selectedRowPeriodic = static_cast<int>(entries.size()) - 1;
            }
        }
    }

    void I2CTableViewModel::CopyPeriodicEntry()
    {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowPeriodic;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }

        PeriodicTriggerEntry copy = entries[index];
        copy.errorCountNAK = 0;
        copy.errorCountCRC = 0;
        entries.insert(entries.begin() + index + 1, copy);
        m_data.selectedRowPeriodic = index + 1;
    }

    void I2CTableViewModel::MovePeriodicEntryUp()
    {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        int index = m_data.selectedRowPeriodic;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowPeriodic--;
        }
    }

    void I2CTableViewModel::MovePeriodicEntryDown()
    {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        int index = m_data.selectedRowPeriodic;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowPeriodic++;
        }
    }

    void I2CTableViewModel::ExecutePeriodicCommand(int index)
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup1();
        if (index < 0 || index >= static_cast<int>(group.periodicTriggerEntries.size())) return;

        const auto& entry = group.periodicTriggerEntries[index];
        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : group.slaveAddress;

        // 【修改】追加 CRC 参数
        switch (entry.type) {
        case CommandType::Read:
            m_hardwareService->InsertSingleRead(slaveAddr, entry.regAddress, entry.length, 3, index, group.crcEnabled, group.crcType);
            break;
        case CommandType::Write:
            m_hardwareService->InsertSingleWrite(slaveAddr, entry.regAddress, entry.data, 3, index, group.crcEnabled, group.crcType);
            break;
        case CommandType::SendCommand:
            m_hardwareService->InsertSingleCommand(slaveAddr, entry.regAddress, 3, index, group.crcEnabled, group.crcType);
            break;
        }
    }

    void I2CTableViewModel::StartPeriodicExecution()
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup1();

        // ========== 动态创建波形图通道 ==========
        if (m_plotViewModel) {
            m_plotViewModel->ClearChannels(); // 每次开始前清空旧通道

            m_plotViewModel->ResetTime();

            m_plotViewModel->SetSystemRunning(true);

            std::vector<ImVec4> presetColors = {
                ImVec4(1.0f, 0.2f, 0.2f, 1.0f), // 红
                ImVec4(0.2f, 0.8f, 0.2f, 1.0f), // 绿
                ImVec4(0.2f, 0.5f, 1.0f, 1.0f), // 蓝
                ImVec4(1.0f, 0.8f, 0.0f, 1.0f), // 黄
                ImVec4(0.8f, 0.2f, 1.0f, 1.0f)  // 紫
            };
            int colorIndex = 0;

            for (size_t i = 0; i < group.periodicTriggerEntries.size(); ++i) {
                const auto& entry = group.periodicTriggerEntries[i];

                if (entry.enabled && entry.plotEnabled &&
                    entry.type == CommandType::Read && entry.parseConfig.enabled)
                {
                    std::string chName = entry.parseConfig.alias.empty() ?
                        ("CH" + std::to_string(i + 1)) :
                        entry.parseConfig.alias;

                    ImVec4 color = presetColors[colorIndex % presetColors.size()];
                    colorIndex++;

                    m_plotViewModel->AddChannel(i, chName, color);
                }
            }
        }
        // ===============================================

        m_data.isPeriodicRunning = true;
        m_hardwareService->EnableBatchMode(true);
        // 【修改】追加 CRC 参数
        m_hardwareService->StartPeriodicExecution(group.slaveAddress, group.periodicTriggerEntries, group.interval, group.crcEnabled, group.crcType);
    }

    void I2CTableViewModel::StopPeriodicExecution()
    {
        if (m_plotViewModel) {
            m_plotViewModel->SetSystemRunning(false);
        }

        m_data.isPeriodicRunning = false;
        m_hardwareService->StopPeriodicExecution();
        m_hardwareService->EnableBatchMode(false);
    }

    void I2CTableViewModel::SetAllPeriodicEntriesEnabled(bool enabled)
    {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        for (auto& entry : entries) {
            entry.enabled = enabled;
        }
    }

    bool I2CTableViewModel::AreAllPeriodicEntriesEnabled() const
    {
        const auto& entries = GetCurrentGroup().periodicTriggerEntries;
        if (entries.empty()) return false;
        for (const auto& entry : entries) {
            if (!entry.enabled) return false;
        }
        return true;
    }

    bool I2CTableViewModel::AreAnyPeriodicEntriesEnabled() const
    {
        const auto& entries = GetCurrentGroup().periodicTriggerEntries;
        for (const auto& entry : entries) {
            if (entry.enabled) return true;
        }
        return false;
    }

    void I2CTableViewModel::ResetPeriodicErrorCounts()
    {
        auto& entries = GetCurrentGroup1().periodicTriggerEntries;
        for (auto& entry : entries) {
            entry.errorCountNAK = 0;
            entry.errorCountCRC = 0;
        }
    }
    // ============== 寄存器表解析方法 ==============

    void I2CTableViewModel::UpdateRegisterParsedValue(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.registerEntries.size()) return;

        auto& entry = group.registerEntries[entryIndex];
        auto& config = entry.parseConfig;

        if (!config.enabled || config.readFormula.empty()) {
            config.parseSuccess = false;
            return;
        }

        if (entry.data.empty()) {
            config.parseSuccess = false;
            config.lastError = "数据为空";
            return;
        }

        auto result = m_expressionParser->EvaluateReadFormula(
            config.readFormula, entry.data);

        config.parsedValue = result.value;
        config.parseSuccess = result.success;
        if (!result.success) {
            config.lastError = result.errorMsg;
        }
    }

    ParseConfig& I2CTableViewModel::GetRegisterParseConfig(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        static ParseConfig emptyConfig;
        if (entryIndex < group.registerEntries.size()) {
            return group.registerEntries[entryIndex].parseConfig;
        }
        return emptyConfig;
    }

    void I2CTableViewModel::SetRegisterParseConfig(size_t entryIndex, const ParseConfig& config) {
        auto& group = GetCurrentGroup1();
        if (entryIndex < group.registerEntries.size()) {
            group.registerEntries[entryIndex].parseConfig = config;
            UpdateRegisterParsedValue(entryIndex);
        }
    }

    // ============== 单次触发解析方法 ==============

    void I2CTableViewModel::UpdateSingleParsedValue(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.singleTriggerEntries.size()) return;

        auto& entry = group.singleTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        if (!config.enabled || config.readFormula.empty()) {
            config.parseSuccess = false;
            return;
        }

        if (entry.data.empty()) {
            config.parseSuccess = false;
            config.lastError = "数据为空";
            return;
        }

        auto result = m_expressionParser->EvaluateReadFormula(
            config.readFormula, entry.data);

        config.parsedValue = result.value;
        config.stringValue = result.stringValue;
        config.parseSuccess = result.success;
        if (!result.success) {
            config.lastError = result.errorMsg;
        }
    }

    // =========================================================
        // 【新增】：单次触发 - 支持字符串的 Raw 数据反算
        // =========================================================
    void I2CTableViewModel::UpdateSingleRawFromParsedValue(size_t entryIndex, const std::string& strValue) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.singleTriggerEntries.size()) return;

        auto& entry = group.singleTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        bool success = false;
        std::string errorMsg;

        // 调用 ExpressionParser 中支持 string 的重载接口
        auto rawData = m_expressionParser->EvaluateWriteFormula(
            config.writeFormula,
            strValue,
            entry.length,
            success,
            errorMsg);

        if (success) {
            entry.data = rawData;
            config.stringValue = strValue; // 更新配置里的 string 值
            config.parseSuccess = true;
        }
        else {
            config.lastError = errorMsg;
            config.parseSuccess = false;
        }
    }

    void I2CTableViewModel::UpdateSingleRawFromParsedValue(size_t entryIndex, double newValue) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.singleTriggerEntries.size()) return;

        auto& entry = group.singleTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        bool success = false;
        std::string errorMsg;

        // 调用 ExpressionParser 中支持 string 的重载接口
        auto rawData = m_expressionParser->EvaluateWriteFormula(
            config.writeFormula,
            newValue,
            entry.length,
            success,
            errorMsg);

        if (success) {
            entry.data = rawData;
            config.parsedValue = newValue; // 更新配置里的 string 值
            config.parseSuccess = true;
        }
        else {
            config.lastError = errorMsg;
            config.parseSuccess = false;
        }
    }
    ParseConfig& I2CTableViewModel::GetSingleParseConfig(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        static ParseConfig emptyConfig;
        if (entryIndex < group.singleTriggerEntries.size()) {
            return group.singleTriggerEntries[entryIndex].parseConfig;
        }
        return emptyConfig;
    }

    void I2CTableViewModel::SetSingleParseConfig(size_t entryIndex, const ParseConfig& config) {
        auto& group = GetCurrentGroup1();
        if (entryIndex < group.singleTriggerEntries.size()) {
            group.singleTriggerEntries[entryIndex].parseConfig = config;
            UpdateSingleParsedValue(entryIndex);
        }
    }

    // ============== 周期触发解析方法（更新） ==============

    void I2CTableViewModel::UpdateParsedValue(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.periodicTriggerEntries.size()) return;

        auto& entry = group.periodicTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        if (!config.enabled || config.readFormula.empty()) {
            config.parseSuccess = false;
            return;
        }

        if (entry.data.empty()) {
            config.parseSuccess = false;
            config.lastError = "数据为空";
            return;
        }

        auto result = m_expressionParser->EvaluateReadFormula(
            config.readFormula, entry.data);

        config.parsedValue = result.value;
        config.stringValue = result.stringValue;
        config.parseSuccess = result.success;
        if (!result.success) {
            config.lastError = result.errorMsg;
        }
        else if (!m_plotViewModel->IsUserPaused()) {
            float currentTime = m_plotViewModel->GetRelativeTime();
            m_plotViewModel->AddDataPoint(entryIndex, currentTime, static_cast<float>(config.parsedValue));
        }
    }

    void I2CTableViewModel::UpdateRawFromParsedValue(size_t entryIndex, double newValue) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.periodicTriggerEntries.size()) return;

        auto& entry = group.periodicTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        bool success = false;
        std::string errorMsg;

        auto rawData = m_expressionParser->EvaluateWriteFormula(
            config.writeFormula,
            newValue,
            entry.length,
            success,
            errorMsg);

        if (success) {
            entry.data = rawData;
            config.parsedValue = newValue;
            config.parseSuccess = true;
        }
        else {
            config.lastError = errorMsg;
            config.parseSuccess = false;
        }
    }

    void I2CTableViewModel::UpdateRawFromParsedValue(size_t entryIndex, const std::string& strValue) {
        auto& group = GetCurrentGroup1();
        if (entryIndex >= group.periodicTriggerEntries.size()) return;

        auto& entry = group.periodicTriggerEntries[entryIndex];
        auto& config = entry.parseConfig;

        bool success = false;
        std::string errorMsg;

        auto rawData = m_expressionParser->EvaluateWriteFormula(
            config.writeFormula,
            strValue,
            entry.length,
            success,
            errorMsg);

        if (success) {
            entry.data = rawData;
            config.stringValue = strValue;
            config.parseSuccess = true;
        }
        else {
            config.lastError = errorMsg;
            config.parseSuccess = false;
        }
    }
    ParseConfig& I2CTableViewModel::GetParseConfig(size_t entryIndex) {
        auto& group = GetCurrentGroup1();
        static ParseConfig emptyConfig;
        if (entryIndex < group.periodicTriggerEntries.size()) {
            return group.periodicTriggerEntries[entryIndex].parseConfig;
        }
        return emptyConfig;
    }

    void I2CTableViewModel::SetParseConfig(size_t entryIndex, const ParseConfig& config) {
        auto& group = GetCurrentGroup1();
        if (entryIndex < group.periodicTriggerEntries.size()) {
            group.periodicTriggerEntries[entryIndex].parseConfig = config;
            UpdateParsedValue(entryIndex);
        }
    }

    std::string I2CTableViewModel::GetFormulaHelp() const {
        return ExpressionParser::GetFormulaHelp();
    }
    // ============== 新增结束 ==============

    uint8_t I2CTableViewModel::ParseHexInput(const char* input) const
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

    std::string I2CTableViewModel::FormatHexData(const std::vector<uint8_t>& data) const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < data.size(); i++) {
            if (i > 0) oss << " ";
            oss << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    std::vector<uint8_t> I2CTableViewModel::ParseHexDataInput(const char* input) const
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
                    //忽略解析错误
                }
            }
        }
        return result;
    }

    void I2CTableViewModel::OnDataResult(const ResponsePacket& packet)
    {
        if (packet.controlId == 0) return;

        m_data.activityIndicator.Trigger();
        auto& group = GetCurrentGroup1();

        switch (packet.controlId) {
        case 1: {  // 寄存器表
            if (packet.commandId < group.registerEntries.size()) {
                auto& entry = group.registerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                entry.data = packet.rawData;
                if (packet.success) {
                    entry.lastError.clear();

                    // 新增：读取成功后自动更新解析值
                    if (entry.parseConfig.enabled && !entry.parseConfig.readFormula.empty()) {
                        UpdateRegisterParsedValue(packet.commandId);
                    }
                }
                else {
                    entry.lastError = packet.errorMsg;
                }
            }
            if (packet.commandId >= group.registerEntries.size() - 1 || !packet.success) {
                m_data.isReadingAllRegisters = false;
            }
            break;
        }
        case 2: {  // 单次触发
            if (packet.commandId < group.singleTriggerEntries.size()) {
                auto& entry = group.singleTriggerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                if(entry.type == CommandType::Read)
                    entry.data = packet.rawData;
                if (packet.success && !packet.rawData.empty()) {
                    entry.lastError.clear();

                    // 新增：读取成功后自动更新解析值
                    if (entry.parseConfig.enabled && !entry.parseConfig.readFormula.empty()) {
                        UpdateSingleParsedValue(packet.commandId);
                    }
                }
                else if (!packet.success) {
                    entry.lastError = packet.errorMsg;
                }
            }
            uint32_t last_enabled_ID = group.GetLastEnabledSingleTriggerID();
            if (last_enabled_ID != UINT32_MAX && packet.commandId >= last_enabled_ID) {
                m_data.isExecuteAllSingleCommands = false;
            }
            break;
        }
        case 3: {  // 周期触发
            if (packet.commandId < group.periodicTriggerEntries.size()) {
                auto& entry = group.periodicTriggerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                if (entry.type == CommandType::Read)
                    entry.data = packet.rawData;
                if (!packet.rawData.empty()) {
                    entry.lastError.clear();

                    // 读取成功后自动更新解析值
                    if (entry.parseConfig.enabled && !entry.parseConfig.readFormula.empty()) {
                        UpdateParsedValue(packet.commandId);
                    }

                }
                if (!packet.success) {
                    entry.lastError = packet.errorMsg;
                    if (packet.errorType == ErrorType::SlaveNotResponse) {
                        entry.errorCountNAK++;
                    }
                    else if (packet.errorType == ErrorType::CRCNotCorrect) {
                        entry.errorCountCRC++;
                    }
                }

                uint32_t last_enabled_ID = group.GetLastEnabledPeriodicTriggerID();
                if (last_enabled_ID != UINT32_MAX && packet.commandId >= last_enabled_ID) {
                    // 新增：记录日志
                    if (m_dataLogger->IsActive()) {
                        m_dataLogger->LogPeriodicRow(group.periodicTriggerEntries);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    void I2CTableViewModel::UndoLastDelete() {
        if (m_undoStack.empty()) return;

        // 弹出最后一个删除动作
        UndoAction action = m_undoStack.back();
        m_undoStack.pop_back();

        auto& data = GetData();
        // 如果组已经被删了，或者索引越界，就放弃撤销
        if (action.groupIndex < 0 || action.groupIndex >= static_cast<int>(data.commandGroups.size())) {
            return;
        }

        auto& group = data.commandGroups[action.groupIndex];

        // 根据类型插回原位置，并自动选中被恢复的行
        if (action.type == UndoItemType::Register) {
            if (action.itemIndex <= static_cast<int>(group.registerEntries.size())) {
                group.registerEntries.insert(group.registerEntries.begin() + action.itemIndex, action.regEntry);
                data.currentGroupIndex = action.groupIndex;
                data.currentTab = TabType::RegisterTable;
                data.selectedRowRegister = action.itemIndex;
            }
        }
        else if (action.type == UndoItemType::SingleTrigger) {
            if (action.itemIndex <= static_cast<int>(group.singleTriggerEntries.size())) {
                group.singleTriggerEntries.insert(group.singleTriggerEntries.begin() + action.itemIndex, action.singleEntry);
                data.currentGroupIndex = action.groupIndex;
                data.currentTab = TabType::SingleTrigger;
                data.selectedRowSingle = action.itemIndex;
            }
        }
        else if (action.type == UndoItemType::PeriodicTrigger) {
            if (action.itemIndex <= static_cast<int>(group.periodicTriggerEntries.size())) {
                group.periodicTriggerEntries.insert(group.periodicTriggerEntries.begin() + action.itemIndex, action.periodicEntry);
                data.currentGroupIndex = action.groupIndex;
                data.currentTab = TabType::PeriodicTrigger;
                data.selectedRowPeriodic = action.itemIndex;
            }
        }
    }
}

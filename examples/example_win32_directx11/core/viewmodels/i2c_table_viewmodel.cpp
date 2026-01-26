#include "i2c_table_viewmodel.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace I2CDebugger {

    I2CTableViewModel::I2CTableViewModel(std::shared_ptr<HardwareService> hardwareService)
        : m_hardwareService(hardwareService)
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

    CommandGroup& I2CTableViewModel::GetCurrentGroup()
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
            GetCurrentGroup().name = newName;
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

    //导出当前命令表
    bool I2CTableViewModel::ExportGroup(const std::string& filePath) {
        return m_configService->SaveCommandGroup(m_data, m_data.currentGroupIndex, filePath);
    }

    // 导入命令表（作为新的命令组添加）
    bool I2CTableViewModel::ImportGroup(const std::string& filePath) {
        return m_configService->LoadCommandGroup(m_data, filePath, true);
    }

    //寄存器表操作
    void I2CTableViewModel::AddRegisterEntry()
    {
        RegisterEntry entry;
        entry.regAddress = 0x00;
        entry.length = 1;
        GetCurrentGroup().registerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeleteRegisterEntry()
    {
        auto& entries = GetCurrentGroup().registerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowRegister;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }
        entries.erase(entries.begin() + index);

        if (m_data.selectedRowRegister >= static_cast<int>(entries.size())) {
            m_data.selectedRowRegister = static_cast<int>(entries.size()) - 1;
        }
    }

    void I2CTableViewModel::CopyRegisterEntry()
    {
        auto& entries = GetCurrentGroup().registerEntries;
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
        auto& entries = GetCurrentGroup().registerEntries;
        int index = m_data.selectedRowRegister;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowRegister--;
        }
    }

    void I2CTableViewModel::MoveRegisterEntryDown()
    {
        auto& entries = GetCurrentGroup().registerEntries;
        int index = m_data.selectedRowRegister;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowRegister++;
        }
    }

    void I2CTableViewModel::ReadAllRegisters()
    {
        if (!m_data.isConnected || m_data.isReadingAllRegisters) {
            return;  // 未连接或正在读取时不执行
        }
        auto& group = GetCurrentGroup();
        if (group.registerEntries.empty()) {
            return;
        }
        // 设置正在读取标志
        m_data.isReadingAllRegisters = true;
        m_hardwareService->ReadAllRegisters(group.slaveAddress, group.registerEntries);
    }

    // 单次触发操作
    void I2CTableViewModel::AddSingleEntry()
    {
        SingleTriggerEntry entry;
        entry.regAddress = 0x00;
        entry.length = 1;
        GetCurrentGroup().singleTriggerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeleteSingleEntry()
    {
        auto& entries = GetCurrentGroup().singleTriggerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowSingle;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }
        entries.erase(entries.begin() + index);

        if (m_data.selectedRowSingle >= static_cast<int>(entries.size())) {
            m_data.selectedRowSingle = static_cast<int>(entries.size()) - 1;
        }
    }

    void I2CTableViewModel::CopySingleEntry()
    {
        auto& entries = GetCurrentGroup().singleTriggerEntries;
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
        auto& entries = GetCurrentGroup().singleTriggerEntries;
        int index = m_data.selectedRowSingle;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowSingle--;
        }
    }

    void I2CTableViewModel::MoveSingleEntryDown()
    {
        auto& entries = GetCurrentGroup().singleTriggerEntries;
        int index = m_data.selectedRowSingle;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowSingle++;
        }
    }

    void I2CTableViewModel::ExecuteSingleCommand(int index)
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup();
        if (index < 0 || index >= static_cast<int>(group.singleTriggerEntries.size())) return;

        const auto& entry = group.singleTriggerEntries[index];
        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : group.slaveAddress;

        switch (entry.type) {
        case CommandType::Read:
            m_hardwareService->InsertSingleRead(slaveAddr, entry.regAddress, entry.length, 2, index);
            break;
        case CommandType::Write:
            m_hardwareService->InsertSingleWrite(slaveAddr, entry.regAddress, entry.data, 2, index);
            break;
        case CommandType::SendCommand:
            m_hardwareService->InsertSingleCommand(slaveAddr, entry.regAddress, 2, index);
            break;
        }
    }

    void I2CTableViewModel::ExecuteAllSingleCommands()
    {
        if (!m_data.isConnected || m_data.isExecuteAllSingleCommands) {
            return;  // 未连接或正在读取时不执行
        }
        auto& group = GetCurrentGroup();
        if (group.singleTriggerEntries.empty()) {
            return;
        }
        // 设置正在读取标志
        m_data.isExecuteAllSingleCommands = true;

        m_hardwareService->ExecuteAllSingleTrigger(group.slaveAddress, group.singleTriggerEntries);
    }

    // 单次触发全选/全不选
    void I2CTableViewModel::SetAllSingleEntriesEnabled(bool enabled)
    {
        auto& entries = GetCurrentGroup().singleTriggerEntries;
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
        GetCurrentGroup().periodicTriggerEntries.push_back(entry);
    }

    void I2CTableViewModel::DeletePeriodicEntry()
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowPeriodic;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }
        entries.erase(entries.begin() + index);

        if (m_data.selectedRowPeriodic >= static_cast<int>(entries.size())) {
            m_data.selectedRowPeriodic = static_cast<int>(entries.size()) - 1;
        }
    }

    void I2CTableViewModel::CopyPeriodicEntry()
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
        if (entries.empty()) return;

        int index = m_data.selectedRowPeriodic;
        if (index < 0 || index >= static_cast<int>(entries.size())) {
            index = static_cast<int>(entries.size()) - 1;
        }

        PeriodicTriggerEntry copy = entries[index];
        copy.errorCount = 0;  // 复制时重置错误计数
        entries.insert(entries.begin() + index + 1, copy);
        m_data.selectedRowPeriodic = index + 1;
    }

    void I2CTableViewModel::MovePeriodicEntryUp()
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
        int index = m_data.selectedRowPeriodic;
        if (index > 0 && index < static_cast<int>(entries.size())) {
            std::swap(entries[index], entries[index - 1]);
            m_data.selectedRowPeriodic--;
        }
    }

    void I2CTableViewModel::MovePeriodicEntryDown()
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
        int index = m_data.selectedRowPeriodic;
        if (index >= 0 && index < static_cast<int>(entries.size()) - 1) {
            std::swap(entries[index], entries[index + 1]);
            m_data.selectedRowPeriodic++;
        }
    }

    void I2CTableViewModel::ExecutePeriodicCommand(int index)
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup();
        if (index < 0 || index >= static_cast<int>(group.periodicTriggerEntries.size())) return;

        const auto& entry = group.periodicTriggerEntries[index];
        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : group.slaveAddress;

        switch (entry.type) {
        case CommandType::Read:
            m_hardwareService->InsertSingleRead(slaveAddr, entry.regAddress, entry.length, 3, index);
            break;
        case CommandType::Write:
            m_hardwareService->InsertSingleWrite(slaveAddr, entry.regAddress, entry.data, 3, index);
            break;
        case CommandType::SendCommand:
            m_hardwareService->InsertSingleCommand(slaveAddr, entry.regAddress, 3, index);
            break;
        }
    }

    void I2CTableViewModel::StartPeriodicExecution()
    {
        if (!m_data.isConnected) return;
        auto& group = GetCurrentGroup();
        m_data.isPeriodicRunning = true;
        m_hardwareService->StartPeriodicExecution(group.slaveAddress, group.periodicTriggerEntries, group.interval);
    }

    void I2CTableViewModel::StopPeriodicExecution()
    {
        m_data.isPeriodicRunning = false;
        m_hardwareService->StopPeriodicExecution();
    }

    // 周期触发全选/全不选
    void I2CTableViewModel::SetAllPeriodicEntriesEnabled(bool enabled)
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
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

    // 重置周期触发错误计数
    void I2CTableViewModel::ResetPeriodicErrorCounts()
    {
        auto& entries = GetCurrentGroup().periodicTriggerEntries;
        for (auto& entry : entries) {
            entry.errorCount = 0;
        }
    }

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

        // 触发活动指示灯
        m_data.activityIndicator.Trigger();

        auto& group = GetCurrentGroup();

        switch (packet.controlId) {
        case 1: {
            if (packet.commandId < group.registerEntries.size()) {
                auto& entry = group.registerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                if (packet.success) {
                    entry.data = packet.rawData;
                    entry.lastError.clear();
                }
                else {
                    entry.lastError = packet.errorMsg;
                }
            }
            // 检查是否是最后一条命令，如果是则重置读取状态
            if (packet.commandId >= group.registerEntries.size() - 1 || !packet.success) {
                m_data.isReadingAllRegisters = false;
            }
            break;
        }
        case 2: {
            if (packet.commandId < group.singleTriggerEntries.size()) {
                auto& entry = group.singleTriggerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                if (packet.success && !packet.rawData.empty()) {
                    entry.data = packet.rawData;
                    entry.lastError.clear();
                }
                else if (!packet.success) {
                    entry.lastError = packet.errorMsg;
                }
            }
            // 检查是否是最后一条命令，如果是则重置读取状态
            if (packet.commandId >= group.singleTriggerEntries.size() - 1 || !packet.success) {
                m_data.isExecuteAllSingleCommands = false;
            }
            break;
        }
        case 3: {
            if (packet.commandId < group.periodicTriggerEntries.size()) {
                auto& entry = group.periodicTriggerEntries[packet.commandId];
                entry.lastSuccess = packet.success;
                entry.lastErrorType = packet.errorType;
                if (packet.success && !packet.rawData.empty()) {
                    entry.data = packet.rawData;
                    entry.lastError.clear();
                }
                else if (!packet.success) {
                    entry.lastError = packet.errorMsg;// NAK错误时增加错误计数
                    if (packet.errorType == ErrorType::SlaveNotResponse) {
                        entry.errorCount++;
                    }
                }
            }

            break;
        }
        default:
            break;
        }
    }

}

#pragma once

#include "../models/i2c_table_app.h"
#include "../services/hardware_service.h"
#include "../services/configuration_service.h"
#include <memory>
#include <string>

namespace I2CDebugger {

    class ConfigurationService;

    class I2CTableViewModel {
    public:
        explicit I2CTableViewModel(std::shared_ptr<HardwareService> hardwareService);

        void Connect();
        void Disconnect();

        void AddGroup();
        void RenameGroup(const std::string& newName);
        void DeleteGroup();

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

        CommandGroup& GetCurrentGroup();
        const CommandGroup& GetCurrentGroup() const;

        I2CTableAppData& GetData() { return m_data; }
        const I2CTableAppData& GetData() const { return m_data; }

        uint8_t ParseHexInput(const char* input) const;
        std::string FormatHexData(const std::vector<uint8_t>& data) const;
        std::vector<uint8_t> ParseHexDataInput(const char* input) const;
        void OnDataResult(const ResponsePacket& packet);

    private:
        I2CTableAppData m_data;
        std::shared_ptr<HardwareService> m_hardwareService;
        std::shared_ptr<ConfigurationService> m_configService;
    };

}

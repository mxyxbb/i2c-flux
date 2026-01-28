#pragma once
#include "../models/i2c_simple_app.h"
#include "../models/i2c_table_app.h"
#include "../models/i2c_command.h"
#include <string>
#include <cstring>
#include "core/nlohmann/json.hpp"

namespace I2CDebugger {

    class ConfigurationService {
    public:
        ConfigurationService() = default;
        ~ConfigurationService() = default;

        // ========== 全局配置（同时保存 Simple 和 Table 数据）==========
        bool SaveGlobalConfiguration(
            const I2CSimpleAppData& simpleData,
            const I2CTableAppData& tableData,
            const std::string& filePath);

        bool LoadGlobalConfiguration(
            I2CSimpleAppData& simpleData,
            I2CTableAppData& tableData,
            const std::string& filePath);

        // ========== 单个命令组导出/导入 ==========
        bool ExportCommandGroup(const I2CTableAppData& data, int groupIndex, const std::string& filePath);
        bool ImportCommandGroup(I2CTableAppData& data, const std::string& filePath, bool asNewGroup);

        std::string GetLastError() const { return m_lastError; }

    private:
        std::string m_lastError;

        // JSON 序列化辅助方法
        nlohmann::json ParseConfigToJson(const ParseConfig& config);
        ParseConfig JsonToParseConfig(const nlohmann::json& j);

        nlohmann::json DataLogConfigToJson(const DataLogConfig& config);
        DataLogConfig JsonToDataLogConfig(const nlohmann::json& j);

        nlohmann::json RegisterEntryToJson(const RegisterEntry& entry);
        RegisterEntry JsonToRegisterEntry(const nlohmann::json& j);

        nlohmann::json SingleTriggerEntryToJson(const SingleTriggerEntry& entry);
        SingleTriggerEntry JsonToSingleTriggerEntry(const nlohmann::json& j);

        nlohmann::json PeriodicTriggerEntryToJson(const PeriodicTriggerEntry& entry);
        PeriodicTriggerEntry JsonToPeriodicTriggerEntry(const nlohmann::json& j);

        nlohmann::json CommandGroupToJson(const CommandGroup& group);
        CommandGroup JsonToCommandGroup(const nlohmann::json& j);

        // Simple 数据序列化
        nlohmann::json SimpleDataToJson(const I2CSimpleAppData& data);
        void JsonToSimpleData(const nlohmann::json& j, I2CSimpleAppData& data);

        // Table 数据序列化
        nlohmann::json TableDataToJson(const I2CTableAppData& data);
        void JsonToTableData(const nlohmann::json& j, I2CTableAppData& data);
    };
}

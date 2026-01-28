#include "configuration_service.h"
#include <fstream>
#include <iomanip>

namespace I2CDebugger {

    using json = nlohmann::json;

    // ============== ParseConfig 序列化 ==============

    json ConfigurationService::ParseConfigToJson(const ParseConfig& config) {
        json j;
        j["enabled"] = config.enabled;
        j["readFormula"] = config.readFormula;
        j["writeFormula"] = config.writeFormula;
        // 如果有 alias 字段
        // j["alias"] = config.alias;
        return j;
    }

    ParseConfig ConfigurationService::JsonToParseConfig(const json& j) {
        ParseConfig config;
        if (j.contains("enabled")) config.enabled = j["enabled"].get<bool>();
        if (j.contains("readFormula")) config.readFormula = j["readFormula"].get<std::string>();
        if (j.contains("writeFormula")) config.writeFormula = j["writeFormula"].get<std::string>();
        // if (j.contains("alias")) config.alias = j["alias"].get<std::string>();
        return config;
    }

    // ============== DataLogConfig 序列化 ==============

    json ConfigurationService::DataLogConfigToJson(const DataLogConfig& config) {
        return json{
            {"enabled", config.enabled},
            {"filePath", config.filePath},
            {"useAlias", config.useAlias},
            {"includeTimestamp", config.includeTimestamp},
            {"logRawData", config.logRawData},
            {"logParsedValue", config.logParsedValue}
        };
    }

    DataLogConfig ConfigurationService::JsonToDataLogConfig(const json& j) {
        DataLogConfig config;
        if (j.contains("enabled")) config.enabled = j["enabled"].get<bool>();
        if (j.contains("filePath")) config.filePath = j["filePath"].get<std::string>();
        if (j.contains("useAlias")) config.useAlias = j["useAlias"].get<bool>();
        if (j.contains("includeTimestamp")) config.includeTimestamp = j["includeTimestamp"].get<bool>();
        if (j.contains("logRawData")) config.logRawData = j["logRawData"].get<bool>();
        if (j.contains("logParsedValue")) config.logParsedValue = j["logParsedValue"].get<bool>();
        return config;
    }

    // ============== RegisterEntry 序列化 ==============

    json ConfigurationService::RegisterEntryToJson(const RegisterEntry& entry) {
        json j;
        j["regAddress"] = entry.regAddress;
        j["length"] = entry.length;
        j["description"] = entry.description;
        j["overrideSlaveAddr"] = entry.overrideSlaveAddr;
        j["slaveAddress"] = entry.slaveAddress;
        j["parseConfig"] = ParseConfigToJson(entry.parseConfig);
        return j;
    }

    RegisterEntry ConfigurationService::JsonToRegisterEntry(const json& j) {
        RegisterEntry entry;
        if (j.contains("regAddress")) entry.regAddress = j["regAddress"].get<uint8_t>();
        if (j.contains("length")) entry.length = j["length"].get<uint8_t>();
        if (j.contains("description")) entry.description = j["description"].get<std::string>();
        if (j.contains("overrideSlaveAddr")) entry.overrideSlaveAddr = j["overrideSlaveAddr"].get<bool>();
        if (j.contains("slaveAddress")) entry.slaveAddress = j["slaveAddress"].get<uint8_t>();
        if (j.contains("parseConfig")) entry.parseConfig = JsonToParseConfig(j["parseConfig"]);
        return entry;
    }

    // ============== SingleTriggerEntry 序列化 ==============

    json ConfigurationService::SingleTriggerEntryToJson(const SingleTriggerEntry& entry) {
        json j;
        j["enabled"] = entry.enabled;
        j["regAddress"] = entry.regAddress;
        j["length"] = entry.length;
        j["delayMs"] = entry.delayMs;
        j["type"] = static_cast<int>(entry.type);
        j["buttonName"] = entry.buttonName;
        j["overrideSlaveAddr"] = entry.overrideSlaveAddr;
        j["slaveAddress"] = entry.slaveAddress;
        j["parseConfig"] = ParseConfigToJson(entry.parseConfig);

        if (!entry.data.empty()) {
            j["data"] = entry.data;
        }
        return j;
    }

    SingleTriggerEntry ConfigurationService::JsonToSingleTriggerEntry(const json& j) {
        SingleTriggerEntry entry;
        if (j.contains("enabled")) entry.enabled = j["enabled"].get<bool>();
        if (j.contains("regAddress")) entry.regAddress = j["regAddress"].get<uint8_t>();
        if (j.contains("length")) entry.length = j["length"].get<uint8_t>();
        if (j.contains("delayMs")) entry.delayMs = j["delayMs"].get<uint16_t>();
        if (j.contains("type")) entry.type = static_cast<CommandType>(j["type"].get<int>());
        if (j.contains("buttonName")) entry.buttonName = j["buttonName"].get<std::string>();
        if (j.contains("overrideSlaveAddr")) entry.overrideSlaveAddr = j["overrideSlaveAddr"].get<bool>();
        if (j.contains("slaveAddress")) entry.slaveAddress = j["slaveAddress"].get<uint8_t>();
        if (j.contains("parseConfig")) entry.parseConfig = JsonToParseConfig(j["parseConfig"]);
        if (j.contains("data")) entry.data = j["data"].get<std::vector<uint8_t>>();
        return entry;
    }

    // ============== PeriodicTriggerEntry 序列化 ==============

    json ConfigurationService::PeriodicTriggerEntryToJson(const PeriodicTriggerEntry& entry) {
        json j;
        j["enabled"] = entry.enabled;
        j["regAddress"] = entry.regAddress;
        j["length"] = entry.length;
        j["delayMs"] = entry.delayMs;
        j["type"] = static_cast<int>(entry.type);
        j["buttonName"] = entry.buttonName;
        j["overrideSlaveAddr"] = entry.overrideSlaveAddr;
        j["slaveAddress"] = entry.slaveAddress;
        j["parseConfig"] = ParseConfigToJson(entry.parseConfig);
        j["plotEnabled"] = entry.plotEnabled;

        if (!entry.data.empty()) {
            j["data"] = entry.data;
        }
        return j;
    }

    PeriodicTriggerEntry ConfigurationService::JsonToPeriodicTriggerEntry(const json& j) {
        PeriodicTriggerEntry entry;
        if (j.contains("enabled")) entry.enabled = j["enabled"].get<bool>();
        if (j.contains("regAddress")) entry.regAddress = j["regAddress"].get<uint8_t>();
        if (j.contains("length")) entry.length = j["length"].get<uint8_t>();
        if (j.contains("delayMs")) entry.delayMs = j["delayMs"].get<uint16_t>();
        if (j.contains("type")) entry.type = static_cast<CommandType>(j["type"].get<int>());
        if (j.contains("buttonName")) entry.buttonName = j["buttonName"].get<std::string>();
        if (j.contains("overrideSlaveAddr")) entry.overrideSlaveAddr = j["overrideSlaveAddr"].get<bool>();
        if (j.contains("slaveAddress")) entry.slaveAddress = j["slaveAddress"].get<uint8_t>();
        if (j.contains("parseConfig")) entry.parseConfig = JsonToParseConfig(j["parseConfig"]);
        if (j.contains("plotEnabled")) entry.plotEnabled = j["plotEnabled"].get<bool>();
        if (j.contains("data")) entry.data = j["data"].get<std::vector<uint8_t>>();
        return entry;
    }

    // ============== CommandGroup 序列化 ==============

    json ConfigurationService::CommandGroupToJson(const CommandGroup& group) {
        json j;
        j["name"] = group.name;
        j["slaveAddress"] = group.slaveAddress;
        j["interval"] = group.interval;
        j["logConfig"] = DataLogConfigToJson(group.logConfig);

        j["registerEntries"] = json::array();
        for (const auto& entry : group.registerEntries) {
            j["registerEntries"].push_back(RegisterEntryToJson(entry));
        }

        j["singleTriggerEntries"] = json::array();
        for (const auto& entry : group.singleTriggerEntries) {
            j["singleTriggerEntries"].push_back(SingleTriggerEntryToJson(entry));
        }

        j["periodicTriggerEntries"] = json::array();
        for (const auto& entry : group.periodicTriggerEntries) {
            j["periodicTriggerEntries"].push_back(PeriodicTriggerEntryToJson(entry));
        }

        return j;
    }

    CommandGroup ConfigurationService::JsonToCommandGroup(const json& j) {
        CommandGroup group;

        if (j.contains("name")) group.name = j["name"].get<std::string>();
        if (j.contains("slaveAddress")) group.slaveAddress = j["slaveAddress"].get<uint8_t>();
        if (j.contains("interval")) group.interval = j["interval"].get<uint32_t>();
        if (j.contains("logConfig")) group.logConfig = JsonToDataLogConfig(j["logConfig"]);

        if (j.contains("registerEntries")) {
            for (const auto& entryJson : j["registerEntries"]) {
                group.registerEntries.push_back(JsonToRegisterEntry(entryJson));
            }
        }

        if (j.contains("singleTriggerEntries")) {
            for (const auto& entryJson : j["singleTriggerEntries"]) {
                group.singleTriggerEntries.push_back(JsonToSingleTriggerEntry(entryJson));
            }
        }

        if (j.contains("periodicTriggerEntries")) {
            for (const auto& entryJson : j["periodicTriggerEntries"]) {
                group.periodicTriggerEntries.push_back(JsonToPeriodicTriggerEntry(entryJson));
            }
        }

        return group;
    }

    // ============== Simple 数据序列化 ==============

    json ConfigurationService::SimpleDataToJson(const I2CSimpleAppData& data) {
        json j;
        j["baudRate"] = data.baudRate;

        // 保存输入缓冲区的内容（字符串形式）
        j["slaveAddrInput"] = std::string(data.slaveAddrInput);
        j["regAddrInput"] = std::string(data.regAddrInput);
        j["lengthInput"] = std::string(data.lengthInput);
        j["writeDataInput"] = std::string(data.writeDataInput);

        // 不保存运行时状态（isConnected, isScanning, isOperating 等）
        // 不保存 readData（读取结果是运行时数据）
        // 不保存 scannedSlaves（扫描结果是运行时数据）
        // 不保存 activityIndicator（UI状态）

        return j;
    }

    void ConfigurationService::JsonToSimpleData(const json& j, I2CSimpleAppData& data) {
        if (j.contains("baudRate")) {
            data.baudRate = j["baudRate"].get<uint32_t>();
        }

        // 恢复输入缓冲区的内容
        if (j.contains("slaveAddrInput")) {
            std::string str = j["slaveAddrInput"].get<std::string>();
            std::strncpy(data.slaveAddrInput, str.c_str(), sizeof(data.slaveAddrInput) - 1);
            data.slaveAddrInput[sizeof(data.slaveAddrInput) - 1] = '\0';
        }

        if (j.contains("regAddrInput")) {
            std::string str = j["regAddrInput"].get<std::string>();
            std::strncpy(data.regAddrInput, str.c_str(), sizeof(data.regAddrInput) - 1);
            data.regAddrInput[sizeof(data.regAddrInput) - 1] = '\0';
        }

        if (j.contains("lengthInput")) {
            std::string str = j["lengthInput"].get<std::string>();
            std::strncpy(data.lengthInput, str.c_str(), sizeof(data.lengthInput) - 1);
            data.lengthInput[sizeof(data.lengthInput) - 1] = '\0';
        }

        if (j.contains("writeDataInput")) {
            std::string str = j["writeDataInput"].get<std::string>();
            std::strncpy(data.writeDataInput, str.c_str(), sizeof(data.writeDataInput) - 1);
            data.writeDataInput[sizeof(data.writeDataInput) - 1] = '\0';
        }
    }

    // ============== Table 数据序列化 ==============

    json ConfigurationService::TableDataToJson(const I2CTableAppData& data) {
        json j;
        j["baudRate"] = data.baudRate;
        j["currentGroupIndex"] = data.currentGroupIndex;

        j["commandGroups"] = json::array();
        for (const auto& group : data.commandGroups) {
            j["commandGroups"].push_back(CommandGroupToJson(group));
        }

        return j;
    }

    void ConfigurationService::JsonToTableData(const json& j, I2CTableAppData& data) {
        if (j.contains("baudRate")) data.baudRate = j["baudRate"].get<int>();
        if (j.contains("currentGroupIndex")) data.currentGroupIndex = j["currentGroupIndex"].get<int>();

        if (j.contains("commandGroups")) {
            data.commandGroups.clear();
            for (const auto& groupJson : j["commandGroups"]) {
                data.commandGroups.push_back(JsonToCommandGroup(groupJson));
            }
        }

        // 确保至少有一个命令组
        if (data.commandGroups.empty()) {
            data.commandGroups.push_back(CommandGroup());
        }

        // 确保索引有效
        if (data.currentGroupIndex >= static_cast<int>(data.commandGroups.size())) {
            data.currentGroupIndex = 0;
        }
    }

    // ============== 全局配置保存/加载 ==============

    bool ConfigurationService::SaveGlobalConfiguration(
        const I2CSimpleAppData& simpleData,
        const I2CTableAppData& tableData,
        const std::string& filePath)
    {
        try {
            json j;
            j["version"] = "1.0";
            j["simpleData"] = SimpleDataToJson(simpleData);
            j["tableData"] = TableDataToJson(tableData);

            std::ofstream file(filePath);
            if (!file.is_open()) {
                m_lastError = "无法打开文件: " + filePath;
                return false;
            }

            file << std::setw(4) << j << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = std::string("保存失败: ") + e.what();
            return false;
        }
    }

    bool ConfigurationService::LoadGlobalConfiguration(
        I2CSimpleAppData& simpleData,
        I2CTableAppData& tableData,
        const std::string& filePath)
    {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                // 文件不存在不算错误，使用默认配置
                m_lastError = "配置文件不存在，使用默认配置";
                return true;
            }

            json j;
            file >> j;

            if (j.contains("simpleData")) {
                JsonToSimpleData(j["simpleData"], simpleData);
            }

            if (j.contains("tableData")) {
                JsonToTableData(j["tableData"], tableData);
            }

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = std::string("加载失败: ") + e.what();
            return false;
        }
    }

    // ============== 命令组导出/导入 ==============

    bool ConfigurationService::ExportCommandGroup(
        const I2CTableAppData& data,
        int groupIndex,
        const std::string& filePath)
    {
        try {
            if (groupIndex < 0 || groupIndex >= static_cast<int>(data.commandGroups.size())) {
                m_lastError = "无效的命令组索引";
                return false;
            }

            json j;
            j["version"] = "1.0";
            j["type"] = "command_group";
            j["group"] = CommandGroupToJson(data.commandGroups[groupIndex]);

            std::ofstream file(filePath);
            if (!file.is_open()) {
                m_lastError = "无法打开文件: " + filePath;
                return false;
            }

            file << std::setw(4) << j << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = std::string("导出失败: ") + e.what();
            return false;
        }
    }

    bool ConfigurationService::ImportCommandGroup(
        I2CTableAppData& data,
        const std::string& filePath,
        bool asNewGroup)
    {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                m_lastError = "无法打开文件: " + filePath;
                return false;
            }

            json j;
            file >> j;

            if (!j.contains("group")) {
                m_lastError = "无效的命令组文件";
                return false;
            }

            CommandGroup group = JsonToCommandGroup(j["group"]);

            if (asNewGroup) {
                data.commandGroups.push_back(group);
                data.currentGroupIndex = static_cast<int>(data.commandGroups.size()) - 1;
            }
            else {
                if (data.currentGroupIndex >= 0 &&
                    data.currentGroupIndex < static_cast<int>(data.commandGroups.size())) {
                    data.commandGroups[data.currentGroupIndex] = group;
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = std::string("导入失败: ") + e.what();
            return false;
        }
    }

} // namespace I2CDebugger

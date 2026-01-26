#include "configuration_service.h"
#include "../models/i2c_simple_app.h"
#include "../models/i2c_table_app.h"
#include "../models/i2c_command.h"
#include "../nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace I2CDebugger {

    std::shared_ptr<ConfigurationService> ConfigurationService::s_instance;

    std::shared_ptr<ConfigurationService> ConfigurationService::GetInstance() {
        static std::shared_ptr<ConfigurationService> instance(new ConfigurationService());
        return instance;
    }

    // 保存全局配置
    bool ConfigurationService::SaveGlobalConfiguration(const I2CSimpleAppData& simpleData, const I2CTableAppData& tableData,
        const std::string& filePath) {
        try {
            json j;

            // 基本信息
            j["version"] = "1.0";
            j["type"] = "global_config";

            // 简单操作窗口数据
            j["simple"]["baud_rate"] = simpleData.baudRate;
            j["simple"]["slave_addr"] = std::string(simpleData.slaveAddrInput);
            j["simple"]["reg_addr"] = std::string(simpleData.regAddrInput);
            j["simple"]["length"] = std::string(simpleData.lengthInput);
            j["simple"]["write_data"] = std::string(simpleData.writeDataInput);
            j["simple"]["operation_type"] = static_cast<int>(simpleData.operationType);

            // 多命令表数据
            j["table"]["current_group"] = tableData.currentGroupIndex;

            json groups = json::array();
            for (const auto& group : tableData.commandGroups) {
                json g;
                g["name"] = group.name;
                g["slave_address"] = static_cast<int>(group.slaveAddress);
                g["interval"] = group.interval;

                // 寄存器表
                json registers = json::array();
                for (const auto& reg : group.registerEntries) {
                    json r;
                    r["reg_address"] = static_cast<int>(reg.regAddress);
                    r["length"] = static_cast<int>(reg.length);
                    r["description"] = reg.description;
                    r["override_slave_addr"] = reg.overrideSlaveAddr;
                    r["slave_address"] = static_cast<int>(reg.slaveAddress);
                    registers.push_back(r);
                }
                g["registers"] = registers;

                // 单次触发
                json singles = json::array();
                for (const auto& single : group.singleTriggerEntries) {
                    json s;
                    s["enabled"] = single.enabled;
                    s["reg_address"] = static_cast<int>(single.regAddress);
                    s["length"] = static_cast<int>(single.length);
                    s["data"] = single.data;
                    s["delay_ms"] = single.delayMs;
                    s["type"] = static_cast<int>(single.type);
                    s["button_name"] = single.buttonName;
                    s["override_slave_addr"] = single.overrideSlaveAddr;
                    s["slave_address"] = static_cast<int>(single.slaveAddress);
                    singles.push_back(s);
                }
                g["singles"] = singles;

                // 周期触发
                json periodics = json::array();
                for (const auto& periodic : group.periodicTriggerEntries) {
                    json p;
                    p["enabled"] = periodic.enabled;
                    p["reg_address"] = static_cast<int>(periodic.regAddress);
                    p["length"] = static_cast<int>(periodic.length);
                    p["data"] = periodic.data;
                    p["delay_ms"] = periodic.delayMs;
                    p["type"] = static_cast<int>(periodic.type);
                    p["button_name"] = periodic.buttonName;
                    p["override_slave_addr"] = periodic.overrideSlaveAddr;
                    p["slave_address"] = static_cast<int>(periodic.slaveAddress);
                    p["alias"] = periodic.alias;
                    p["formula"] = periodic.formula;
                    periodics.push_back(p);
                }
                g["periodics"] = periodics;

                groups.push_back(g);
            }
            j["table"]["groups"] = groups;

            // 写入文件
            std::ofstream file(filePath);
            file << j.dump(4);
            file.close();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Save failed: " << e.what() << std::endl;
            return false;
        }
    }

    // 加载全局配置
    bool ConfigurationService::LoadGlobalConfiguration(I2CSimpleAppData& simpleData, I2CTableAppData& tableData,
        const std::string& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) return false;

            json j;
            file >> j;
            file.close();

            // 验证类型
            if (j["type"] != "global_config") {
                return false;
            }

            // 恢复简单操作窗口数据
            if (j.contains("simple")) {
                simpleData.baudRate = j["simple"]["baud_rate"];
                std::strncpy(simpleData.slaveAddrInput, j["simple"]["slave_addr"].get<std::string>().c_str(),
                    sizeof(simpleData.slaveAddrInput) - 1);
                std::strncpy(simpleData.regAddrInput, j["simple"]["reg_addr"].get<std::string>().c_str(),
                    sizeof(simpleData.regAddrInput) - 1);
                std::strncpy(simpleData.lengthInput, j["simple"]["length"].get<std::string>().c_str(),
                    sizeof(simpleData.lengthInput) - 1);
                std::strncpy(simpleData.writeDataInput, j["simple"]["write_data"].get<std::string>().c_str(),
                    sizeof(simpleData.writeDataInput) - 1);
                simpleData.operationType = static_cast<OperationType>(j["simple"]["operation_type"]);
            }

            // 恢复多命令表数据
            tableData.commandGroups.clear();
            if (j.contains("table")) {
                tableData.currentGroupIndex = j["table"]["current_group"];

                for (const auto& g : j["table"]["groups"]) {
                    CommandGroup group;
                    group.name = g["name"];
                    group.slaveAddress = static_cast<uint8_t>(g["slave_address"]);
                    group.interval = g["interval"];

                    // 恢复寄存器表
                    for (const auto& r : g["registers"]) {
                        RegisterEntry reg;
                        reg.regAddress = static_cast<uint8_t>(r["reg_address"]);
                        reg.length = static_cast<uint8_t>(r["length"]);
                        reg.description = r["description"];
                        reg.overrideSlaveAddr = r["override_slave_addr"];
                        reg.slaveAddress = static_cast<uint8_t>(r["slave_address"]);
                        group.registerEntries.push_back(reg);
                    }

                    // 恢复单次触发
                    for (const auto& s : g["singles"]) {
                        SingleTriggerEntry single;
                        single.enabled = s["enabled"];
                        single.regAddress = static_cast<uint8_t>(s["reg_address"]);
                        single.length = static_cast<uint8_t>(s["length"]);
                        single.data = s["data"].get<std::vector<uint8_t>>();
                        single.delayMs = s["delay_ms"];
                        single.type = static_cast<CommandType>(s["type"]);
                        single.buttonName = s["button_name"];
                        single.overrideSlaveAddr = s["override_slave_addr"];
                        single.slaveAddress = static_cast<uint8_t>(s["slave_address"]);
                        group.singleTriggerEntries.push_back(single);
                    }

                    // 恢复周期触发
                    for (const auto& p : g["periodics"]) {
                        PeriodicTriggerEntry periodic;
                        periodic.enabled = p["enabled"];
                        periodic.regAddress = static_cast<uint8_t>(p["reg_address"]);
                        periodic.length = static_cast<uint8_t>(p["length"]);
                        periodic.data = p["data"].get<std::vector<uint8_t>>();
                        periodic.delayMs = p["delay_ms"];
                        periodic.type = static_cast<CommandType>(p["type"]);
                        periodic.buttonName = p["button_name"];
                        periodic.overrideSlaveAddr = p["override_slave_addr"];
                        periodic.slaveAddress = static_cast<uint8_t>(p["slave_address"]);
                        periodic.alias = p["alias"];
                        periodic.formula = p["formula"];
                        periodic.parseConfigured = !periodic.alias.empty() || !periodic.formula.empty();
                        group.periodicTriggerEntries.push_back(periodic);
                    }

                    tableData.commandGroups.push_back(group);
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Load failed: " << e.what() << std::endl;
            return false;
        }
    }

    // 保存单个命令表
    bool ConfigurationService::SaveCommandGroup(const I2CTableAppData& tableData, int groupIndex, const std::string& filePath) {
        try {
            if (groupIndex < 0 || groupIndex >= static_cast<int>(tableData.commandGroups.size())) {
                return false;
            }

            const auto& group = tableData.commandGroups[groupIndex];

            json j;
            j["version"] = "1.0";
            j["type"] = "command_group";
            j["name"] = group.name;
            j["slave_address"] = static_cast<int>(group.slaveAddress);
            j["interval"] = group.interval;

            // 寄存器表
            json registers = json::array();
            for (const auto& reg : group.registerEntries) {
                json r;
                r["reg_address"] = static_cast<int>(reg.regAddress);
                r["length"] = static_cast<int>(reg.length);
                r["description"] = reg.description;
                r["override_slave_addr"] = reg.overrideSlaveAddr;
                r["slave_address"] = static_cast<int>(reg.slaveAddress);
                registers.push_back(r);
            }
            j["registers"] = registers;

            // 单次触发
            json singles = json::array();
            for (const auto& single : group.singleTriggerEntries) {
                json s;
                s["enabled"] = single.enabled;
                s["reg_address"] = static_cast<int>(single.regAddress);
                s["length"] = static_cast<int>(single.length);
                s["data"] = single.data;
                s["delay_ms"] = single.delayMs;
                s["type"] = static_cast<int>(single.type);
                s["button_name"] = single.buttonName;
                s["override_slave_addr"] = single.overrideSlaveAddr;
                s["slave_address"] = static_cast<int>(single.slaveAddress);
                singles.push_back(s);
            }
            j["singles"] = singles;

            // 周期触发
            json periodics = json::array();
            for (const auto& periodic : group.periodicTriggerEntries) {
                json p;

                p["enabled"] = periodic.enabled;
                p["reg_address"] = static_cast<int>(periodic.regAddress);
                p["length"] = static_cast<int>(periodic.length);
                p["data"] = periodic.data;
                p["delay_ms"] = periodic.delayMs;
                p["type"] = static_cast<int>(periodic.type);
                p["button_name"] = periodic.buttonName;
                p["override_slave_addr"] = periodic.overrideSlaveAddr;
                p["slave_address"] = static_cast<int>(periodic.slaveAddress);
                p["alias"] = periodic.alias;
                p["formula"] = periodic.formula;

                periodics.push_back(p);
            }
            j["periodics"] = periodics;

            // 写入文件
            std::ofstream file(filePath);
            file << j.dump(4);
            file.close();

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Save command group failed: " << e.what() << std::endl;
            return false;
        }
    }

    // 加载单个命令表
    bool ConfigurationService::LoadCommandGroup(I2CTableAppData& tableData, const std::string& filePath, bool appendAsNew) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) return false;

            json j;
            file >> j;
            file.close();

            // 验证类型
            if (j["type"] != "command_group") {
                return false;
            }

            CommandGroup group;
            group.name = j["name"];
            group.slaveAddress = static_cast<uint8_t>(j["slave_address"]);
            group.interval = j["interval"];

            // 恢复寄存器表
            for (const auto& r : j["registers"]) {
                RegisterEntry reg;
                reg.regAddress = static_cast<uint8_t>(r["reg_address"]);
                reg.length = static_cast<uint8_t>(r["length"]);
                reg.description = r["description"];
                reg.overrideSlaveAddr = r["override_slave_addr"];
                reg.slaveAddress = static_cast<uint8_t>(r["slave_address"]);
                group.registerEntries.push_back(reg);
            }

            // 恢复单次触发
            for (const auto& s : j["singles"]) {
                SingleTriggerEntry single;
                single.enabled = s["enabled"];
                single.regAddress = static_cast<uint8_t>(s["reg_address"]);
                single.length = static_cast<uint8_t>(s["length"]);
                single.data = s["data"].get<std::vector<uint8_t>>();
                single.delayMs = s["delay_ms"];
                single.type = static_cast<CommandType>(s["type"]);
                single.buttonName = s["button_name"];
                single.overrideSlaveAddr = s["override_slave_addr"];
                single.slaveAddress = static_cast<uint8_t>(s["slave_address"]);
                group.singleTriggerEntries.push_back(single);
            }

            // 恢复周期触发
            for (const auto& p : j["periodics"]) {
                PeriodicTriggerEntry periodic;
                periodic.enabled = p["enabled"];
                periodic.regAddress = static_cast<uint8_t>(p["reg_address"]);
                periodic.length = static_cast<uint8_t>(p["length"]);
                periodic.data = p["data"].get<std::vector<uint8_t>>();
                periodic.delayMs = p["delay_ms"];
                periodic.type = static_cast<CommandType>(p["type"]);
                periodic.buttonName = p["button_name"];
                periodic.overrideSlaveAddr = p["override_slave_addr"];
                periodic.slaveAddress = static_cast<uint8_t>(p["slave_address"]);
                periodic.alias = p["alias"];
                periodic.formula = p["formula"];
                periodic.parseConfigured = !periodic.alias.empty() || !periodic.formula.empty();

                group.periodicTriggerEntries.push_back(periodic);
            }

            if (appendAsNew) {
                // 添加为新的命令组
                tableData.commandGroups.push_back(group);
                tableData.currentGroupIndex = static_cast<int>(tableData.commandGroups.size()) - 1;
            }
            else {
                // 替换当前命令组
                if (tableData.currentGroupIndex >= 0 && tableData.currentGroupIndex < static_cast<int>(tableData.commandGroups.size())) {
                    tableData.commandGroups[tableData.currentGroupIndex] = group;
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Load command group failed: " << e.what() << std::endl;
            return false;
        }
    }
}

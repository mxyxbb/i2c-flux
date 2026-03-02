#include "quick_add_processor.h"
#include "i2c_table_viewmodel.h"      // 引入 ViewModel
#include "../models/i2c_table_app.h"  // 引入 CommandGroup 和 Entry 结构体
#include <sstream>
#include <algorithm>

namespace I2CDebugger {

    // ========== 内部辅助函数：解析 CSV 行 ==========
    static std::vector<std::string> ParseCSVLine(const std::string& line) {
        std::vector<std::string> result;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t first = item.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                result.push_back("");
            }
            else {
                size_t last = item.find_last_not_of(" \t\r\n");
                result.push_back(item.substr(first, (last - first + 1)));
            }
        }
        if (!line.empty() && line.back() == ',') result.push_back("");
        return result;
    }

    // ========== 核心处理逻辑 ==========
    void QuickAddProcessor::Process(std::shared_ptr<I2CTableViewModel> viewModel, int tabType, const std::string& csvData) {
        if (!viewModel) return;

        auto& group = viewModel->GetCurrentGroup1();
        std::istringstream iss(csvData);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            auto tokens = ParseCSVLine(line);
            if (tokens.empty() || tokens[0].empty()) continue;

            if (tabType == 1 || tabType == 2) {
                // 单次触发 & 周期触发
                if (tokens.size() < 2) continue;
                std::string typeStr = tokens[0];
                std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

                uint8_t regAddr = viewModel->ParseHexInput(tokens[1].c_str());

                if (typeStr == "read") {
                    uint8_t len = (tokens.size() > 2 && !tokens[2].empty()) ? static_cast<uint8_t>(std::stoi(tokens[2])) : 1;
                    std::string formula = tokens.size() > 3 ? tokens[3] : "";
                    std::string alias = tokens.size() > 4 ? tokens[4] : "";

                    if (tabType == 1) {
                        SingleTriggerEntry singleEntry;
                        singleEntry.type = CommandType::Read;
                        singleEntry.regAddress = regAddr;
                        singleEntry.length = len;
                        singleEntry.parseConfig.readFormula = formula;
                        singleEntry.parseConfig.alias = alias;
                        singleEntry.parseConfig.enabled = !formula.empty();
                        group.singleTriggerEntries.push_back(singleEntry);
                    }
                    else {
                        PeriodicTriggerEntry entry;
                        entry.type = CommandType::Read;
                        entry.regAddress = regAddr;
                        entry.length = len;
                        entry.parseConfig.readFormula = formula;
                        entry.parseConfig.alias = alias;
                        entry.parseConfig.enabled = !formula.empty();
                        group.periodicTriggerEntries.push_back(entry);
                    }
                }
                else if (typeStr == "write") {
                    std::string formula = tokens.size() > 2 ? tokens[2] : "";
                    std::string decValueStr = tokens.size() > 3 ? tokens[3] : "";
                    std::string alias = tokens.size() > 4 ? tokens[4] : "";

                    size_t newIndex = 0;
                    bool isEnabled = !formula.empty();

                    if (tabType == 1) {
                        SingleTriggerEntry singleEntry;
                        singleEntry.type = CommandType::Write;
                        singleEntry.regAddress = regAddr;
                        singleEntry.length = 1;
                        singleEntry.parseConfig.writeFormula = formula;
                        singleEntry.parseConfig.alias = alias;
                        singleEntry.parseConfig.enabled = isEnabled;
                        group.singleTriggerEntries.push_back(singleEntry);
                        newIndex = group.singleTriggerEntries.size() - 1;
                    }
                    else {
                        PeriodicTriggerEntry entry;
                        entry.type = CommandType::Write;
                        entry.regAddress = regAddr;
                        entry.length = 1;
                        entry.parseConfig.writeFormula = formula;
                        entry.parseConfig.alias = alias;
                        entry.parseConfig.enabled = isEnabled;
                        group.periodicTriggerEntries.push_back(entry);
                        newIndex = group.periodicTriggerEntries.size() - 1;
                    }

                    if (!decValueStr.empty() && isEnabled) {
                        try {
                            double val = std::stod(decValueStr);
                            if (tabType == 1) viewModel->UpdateSingleRawFromParsedValue(newIndex, val);
                            else viewModel->UpdateRawFromParsedValue(newIndex, val);
                        }
                        catch (...) {
                            // 忽略 stod 转换失败的情况
                        }
                    }
                }
            }
            else if (tabType == 0) {
                // 寄存器表
                uint8_t regAddr = viewModel->ParseHexInput(tokens[0].c_str());
                uint8_t len = (tokens.size() > 1 && !tokens[1].empty()) ? static_cast<uint8_t>(std::stoi(tokens[1])) : 1;
                std::string formula = tokens.size() > 2 ? tokens[2] : "";
                std::string alias = tokens.size() > 3 ? tokens[3] : "";
                std::string desc = tokens.size() > 4 ? tokens[4] : "";

                RegisterEntry entry;
                entry.regAddress = regAddr;
                entry.length = len;
                entry.parseConfig.readFormula = formula;
                entry.parseConfig.alias = alias;
                entry.parseConfig.enabled = !formula.empty();
                entry.description = desc;

                group.registerEntries.push_back(entry);
            }
        }
    }
}

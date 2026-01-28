#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/nlohmann/json.hpp"

// 命令类型
enum class CmdType { Read, Write, SendCmd };

// 单行命令数据 (用于表格和存储)
struct CommandRow {
    bool enabled = true;
    uint8_t slave_addr_override = 0x00;
    bool use_override = false;
    uint8_t reg_addr = 0x00;
    int length = 1;
    std::string write_hex_string;
    int delay_ms = 0;
    CmdType type = CmdType::Read;
    std::string label_name = "Exec";

    // 运行时显示状态 (不序列化)
    std::string last_val_display;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CommandRow, enabled, use_override, slave_addr_override, reg_addr, length, write_hex_string, delay_ms, type, label_name)
};

// 命令组
struct CommandGroup {
    std::string name = "Default";
    uint8_t global_slave_addr = 0x50;
    std::vector<CommandRow> rows;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CommandGroup, name, global_slave_addr, rows)
};

// 硬件层任务包
struct HWTask {
    bool is_periodic;
    int group_id;     // -1: SimpleWin, >=0: TableWin
    int cmd_index;    // 控件/行ID
    CommandRow cmd_data;
    uint8_t effective_addr;
};

// 硬件层结果包
struct HWResult {
    int group_id;
    int cmd_index;
    bool success;
    std::vector<uint8_t> raw_bytes;
    std::string error_msg;
};

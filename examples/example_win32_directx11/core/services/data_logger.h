#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include "../models/i2c_table_app.h"

namespace I2CDebugger {

    // 周期数据行记录（一次周期触发的所有读取数据）
    struct PeriodicDataRow {
        std::chrono::system_clock::time_point timestamp;
        std::vector<std::pair<uint8_t, std::vector<uint8_t>>> rawDataList;  // <regAddr, rawData>
        std::vector<std::pair<std::string, double>> parsedDataList;          // <alias, parsedValue>
    };

    class DataLogger {
    public:
        DataLogger();
        ~DataLogger();

        // 开始记录 - 传入周期触发条目用于生成表头
        bool Start(const std::string& filePath,
            const std::vector<PeriodicTriggerEntry>& entries,
            const DataLogConfig& config);
        void Stop();

        // 记录一行周期数据（一次完整的周期触发结果）
        void LogPeriodicRow(const std::vector<PeriodicTriggerEntry>& entries);

        // 状态查询
        bool IsActive() const { return m_isActive.load(); }
        uint32_t GetLoggedCount() const { return m_loggedCount.load(); }
        std::string GetLastError() const;

    private:
        void WorkerThread();
        void WriteHeader(const std::vector<PeriodicTriggerEntry>& entries);
        void WriteRow(const PeriodicDataRow& row);

        std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp);
        std::string FormatRawData(const std::vector<uint8_t>& data);
        std::string EscapeCsvField(const std::string& field);

        // 文件
        std::ofstream m_file;
        std::string m_filePath;
        DataLogConfig m_config;

        // 表头信息（记录哪些列需要输出）
        struct ColumnInfo {
            uint8_t regAddress;
            std::string alias;
            bool hasAlias;
        };
        std::vector<ColumnInfo> m_columns;

        // 线程安全
        std::thread m_workerThread;
        std::atomic<bool> m_isActive{ false };
        std::atomic<bool> m_shouldStop{ false };
        std::atomic<uint32_t> m_loggedCount{ 0 };

        std::queue<PeriodicDataRow> m_rowQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_queueCondition;

        mutable std::mutex m_errorMutex;
        std::string m_lastError;
    };

} // namespace I2CDebuggers

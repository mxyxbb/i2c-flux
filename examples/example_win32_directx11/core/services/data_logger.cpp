#include "data_logger.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>

namespace I2CDebugger {

    DataLogger::DataLogger() = default;

    DataLogger::~DataLogger() {
        Stop();
    }

    bool DataLogger::Start(const std::string& filePath,
        const std::vector<PeriodicTriggerEntry>& entries,
        const DataLogConfig& config) {
        // 如果已经在运行，先停止
        if (m_isActive.load()) {
            Stop();
        }

        m_filePath = filePath;
        m_config = config;
        m_loggedCount.store(0);
        m_columns.clear();

        // 收集需要记录的列信息（仅读取类型且启用的命令）
        for (const auto& entry : entries) {
            if (entry.type != CommandType::Read || !entry.enabled) {
                continue;
            }

            ColumnInfo col;
            col.regAddress = entry.regAddress;
            // 如果有别名则使用别名，否则使用 "Parse"
            if (entry.parseConfig.enabled && !entry.parseConfig.alias.empty()) {
                col.alias = entry.parseConfig.alias;
                col.hasAlias = true;
            }
            else {
                col.alias = "Parse";
                col.hasAlias = false;
            }
            m_columns.push_back(col);
        }

        if (m_columns.empty()) {
            std::lock_guard<std::mutex> lock(m_errorMutex);
            m_lastError = "没有可记录的读取命令";
            return false;
        }

        // 打开文件（二进制模式，避免换行符转换问题）
        m_file.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!m_file.is_open()) {
            std::lock_guard<std::mutex> lock(m_errorMutex);
            m_lastError = "无法打开日志文件: " + filePath;
            return false;
        }

        // 写入 UTF-8 BOM（确保 Excel 正确识别中文）
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        m_file.write(reinterpret_cast<const char*>(bom), sizeof(bom));

        // 写入表头
        WriteHeader(entries);

        // 启动工作线程
        m_shouldStop.store(false);
        m_isActive.store(true);
        m_workerThread = std::thread(&DataLogger::WorkerThread, this);

        return true;
    }

    void DataLogger::Stop() {
        if (!m_isActive.load()) {
            return;
        }

        // 通知工作线程停止
        m_shouldStop.store(true);
        m_queueCondition.notify_all();

        // 等待工作线程结束
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }

        // 关闭文件
        if (m_file.is_open()) {
            m_file.flush();
            m_file.close();
        }

        m_isActive.store(false);
        m_columns.clear();
    }

    void DataLogger::LogPeriodicRow(const std::vector<PeriodicTriggerEntry>& entries) {
        if (!m_isActive.load()) return;

        PeriodicDataRow row;
        row.timestamp = std::chrono::system_clock::now();

        // 收集所有读取命令的数据
        size_t colIndex = 0;
        for (const auto& entry : entries) {
            if (entry.type != CommandType::Read || !entry.enabled) {
                continue;
            }

            if (colIndex >= m_columns.size()) {
                break;
            }

            // Raw数据
            row.rawDataList.push_back({ entry.regAddress, entry.data });

            // 解析值
            if (entry.parseConfig.enabled && entry.parseConfig.parseSuccess) {
                row.parsedDataList.push_back({ m_columns[colIndex].alias, entry.parseConfig.parsedValue });
            }
            else {
                // 解析失败或未配置解析，使用 NaN 表示
                row.parsedDataList.push_back({ m_columns[colIndex].alias, std::nan("") });
            }

            colIndex++;
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_rowQueue.push(std::move(row));
        }
        m_queueCondition.notify_one();
    }

    std::string DataLogger::GetLastError() const {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        return m_lastError;
    }

    void DataLogger::WorkerThread() {
        while (!m_shouldStop.load()) {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            m_queueCondition.wait(lock, [this] {
                return !m_rowQueue.empty() || m_shouldStop.load();
                });

            while (!m_rowQueue.empty()) {
                PeriodicDataRow row = std::move(m_rowQueue.front());
                m_rowQueue.pop();
                lock.unlock();

                WriteRow(row);
                m_loggedCount.fetch_add(1);

                lock.lock();
            }
        }

        // 处理剩余数据
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_rowQueue.empty()) {
            PeriodicDataRow row = std::move(m_rowQueue.front());
            m_rowQueue.pop();
            WriteRow(row);
            m_loggedCount.fetch_add(1);
        }
    }

    void DataLogger::WriteHeader(const std::vector<PeriodicTriggerEntry>& entries) {
        if (!m_file.is_open()) return;

        std::ostringstream oss;

        // 时间戳列
        if (m_config.includeTimestamp) {
            oss << "Timestamp";
        }

        // 遍历所有列，每个读取命令生成两列：地址列 + 别名列
        for (const auto& col : m_columns) {
            // 寄存器地址列（Raw数据表头）
            if (m_config.logRawData) {
                if (oss.tellp() > 0) oss << ",";
                char addrStr[16];
                std::snprintf(addrStr, sizeof(addrStr), "0x%02X", col.regAddress);
                oss << addrStr;
            }

            // 别名列（解析值表头）- 无别名时显示 "Parse"
            if (m_config.logParsedValue) {
                if (oss.tellp() > 0) oss << ",";
                oss << EscapeCsvField(col.alias);
            }
        }

        oss << "\r\n";
        m_file << oss.str();
        m_file.flush();
    }

    void DataLogger::WriteRow(const PeriodicDataRow& row) {
        if (!m_file.is_open()) return;

        std::ostringstream oss;

        // 时间戳
        if (m_config.includeTimestamp) {
            oss << FormatTimestamp(row.timestamp);
        }

        // 按列顺序交替写入数据（地址列Raw数据 + 别名列解析值）
        size_t dataIndex = 0;
        for (size_t i = 0; i < m_columns.size(); ++i) {
            // Raw数据列
            if (m_config.logRawData) {
                if (oss.tellp() > 0) oss << ",";
                if (dataIndex < row.rawDataList.size()) {
                    const std::vector<uint8_t>& rawData = row.rawDataList[dataIndex].second;
                    oss << EscapeCsvField(FormatRawData(rawData));
                }
                else {
                    oss << "";
                }
            }

            // 解析值列
            if (m_config.logParsedValue) {
                if (oss.tellp() > 0) oss << ",";
                if (dataIndex < row.parsedDataList.size()) {
                    double value = row.parsedDataList[dataIndex].second;
                    if (std::isnan(value)) {
                        oss << "ERR";
                    }
                    else {
                        oss << std::fixed << std::setprecision(6) << value;
                    }
                }
                else {
                    oss << "ERR";
                }
            }

            dataIndex++;
        }

        oss << "\r\n";
        m_file << oss.str();

        // 定期刷新
        if (m_loggedCount.load() % 10 == 0) {
            m_file.flush();
        }
    }

    std::string DataLogger::FormatTimestamp(const std::chrono::system_clock::time_point& tp) {
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % 1000;

        std::tm tm_val;
#ifdef _WIN32
        localtime_s(&tm_val, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_val);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }

    std::string DataLogger::FormatRawData(const std::vector<uint8_t>& data) {
        std::ostringstream oss;
        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0) oss << " ";
            oss << std::hex << std::uppercase << std::setfill('0')
                << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    std::string DataLogger::EscapeCsvField(const std::string& field) {
        bool needsQuotes = field.find(',') != std::string::npos ||
            field.find('"') != std::string::npos ||
            field.find('\n') != std::string::npos ||
            field.find('\r') != std::string::npos;

        if (!needsQuotes) {
            return field;
        }

        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\"";
            }
            else {
                escaped += c;
            }
        }
        escaped += "\"";

        return escaped;
    }

} // namespace I2CDebugger

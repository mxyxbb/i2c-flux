#pragma once

#include "../models/i2c_command.h"
#include "../models/i2c_table_app.h"    // 添加这行！包含 RegisterEntry, SingleTriggerEntry, PeriodicTriggerEntry
#include "../../hardware/PMBus/pmbus.h"
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace I2CDebugger {

    // ========== 任务类型枚举 ==========
    enum class TaskType {
        Connect,
        Disconnect,
        ScanSlaves,
        ReadRegister,
        WriteRegister,
        SendCommand,
        ReadAllRegisters,
        ExecuteAllCommands,
        StartPeriodic,
        StopPeriodic
    };

    // ========== 硬件任务结构 ==========
    struct HardwareTask {
        TaskType type;
        uint8_t slaveAddr = 0;
        uint8_t regAddr = 0;
        uint8_t length = 0;
        std::vector<uint8_t> data;
        uint32_t controlId = 0;
        uint32_t commandId = 0;
        uint32_t baudRate = BAUD_RATE_100K;
        uint32_t delayMs = 0;
        CommandType cmdType = CommandType::Read;
        std::vector<RegisterEntry> registerEntries;
        std::vector<SingleTriggerEntry> singleEntries;
        std::vector<PeriodicTriggerEntry> periodicEntries;
        uint32_t intervalMs = 100;
    };

    // ========== 回调类型定义 ==========
    using ConnectCallback = std::function<void(bool success, const std::string& deviceName, const std::string& errorMsg)>;
    using DisconnectCallback = std::function<void()>;
    using ScanCallback = std::function<void(bool success, const std::vector<uint8_t>& slaves, const std::string& errorMsg)>;
    using DataCallback = std::function<void(const ResponsePacket& packet)>;

    // ========== 硬件服务类 ==========
    class HardwareService {
    public:
        HardwareService();
        ~HardwareService();

        // 服务控制
        void Start();
        void Stop();

        // 连接管理
        void Connect(uint32_t baudRate);
        void Disconnect();
        void ScanSlaves();

        // 单次操作
        void ReadRegister(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
            uint32_t controlId, uint32_t commandId);
        void WriteRegister(uint8_t slaveAddr, uint8_t regAddr,
            const std::vector<uint8_t>& data,
            uint32_t controlId, uint32_t commandId);
        void SendCommand(uint8_t slaveAddr, uint8_t regAddr,
            uint32_t controlId, uint32_t commandId);

        // 批量操作
        void ReadAllRegisters(uint8_t defaultSlaveAddr, const std::vector<RegisterEntry>& entries);
        void ExecuteAllSingleTrigger(uint8_t defaultSlaveAddr, const std::vector<SingleTriggerEntry>& entries);

        // 周期执行
        void StartPeriodicExecution(uint8_t defaultSlaveAddr,
            const std::vector<PeriodicTriggerEntry>& entries,
            uint32_t intervalMs);
        void StopPeriodicExecution();

        // 优先级插入（周期执行期间的单次操作）
        void InsertSingleRead(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
            uint32_t controlId, uint32_t commandId);
        void InsertSingleWrite(uint8_t slaveAddr, uint8_t regAddr,
            const std::vector<uint8_t>& data,
            uint32_t controlId, uint32_t commandId);
        void InsertSingleCommand(uint8_t slaveAddr, uint8_t regAddr,
            uint32_t controlId, uint32_t commandId);

        // 回调设置
        void SetConnectCallback(ConnectCallback callback) { m_connectCallback = callback; }
        void SetDisconnectCallback(DisconnectCallback callback) { m_disconnectCallback = callback; }
        void SetScanCallback(ScanCallback callback) { m_scanCallback = callback; }
        void SetDataCallback(DataCallback callback) { m_dataCallback = callback; }

        // 在主线程中处理回调
        void ProcessCallbacks();

        // 状态查询
        bool IsConnected() const { return m_isConnected; }
        bool IsPeriodicRunning() const { return m_periodicRunning; }

    private:
        // 工作线程
        void WorkerThread();
        void ProcessTask(const HardwareTask& task);
        void ExecutePeriodicTask();
        void ProcessPriorityTasks();
        void HandleDeviceDisconnected();
        ErrorType GetErrorType(int returnValue);  // 修复：分开两行

        // 工作线程
        std::thread m_workerThread;               // 修复：单独一行
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_isConnected{ false };
        std::atomic<bool> m_periodicRunning{ false };

        // 任务队列
        std::queue<HardwareTask> m_taskQueue;
        std::queue<HardwareTask> m_priorityQueue;
        std::mutex m_taskMutex;
        std::condition_variable m_taskCv;

        // 回调函数
        ConnectCallback m_connectCallback;
        DisconnectCallback m_disconnectCallback;
        ScanCallback m_scanCallback;
        DataCallback m_dataCallback;

        // 回调队列（用于线程安全的回调）
        std::queue<std::function<void()>> m_callbackQueue;
        std::mutex m_callbackMutex;

        // 周期执行数据
        std::vector<PeriodicTriggerEntry> m_periodicEntries;
        uint8_t m_periodicSlaveAddr = 0;
        uint32_t m_periodicIntervalMs = 100;

        // 硬件设备
        PMBus m_pmbus;
        std::mutex m_deviceMutex;
    };

} // namespace I2CDebugger

#pragma once

#include "../models/i2c_command.h"
#include "../../hardware/PMBus/pmbus.h"
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace I2CDebugger {

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

    using ConnectCallback = std::function<void(bool success, const std::string& deviceName, const std::string& errorMsg)>;
    using DisconnectCallback = std::function<void()>;
    using ScanCallback = std::function<void(bool success, const std::vector<uint8_t>& slaves, const std::string& errorMsg)>;
    using DataCallback = std::function<void(const ResponsePacket& packet)>;

    class HardwareService {
    public:
        HardwareService();
        ~HardwareService();

        void Start();
        void Stop();

        void Connect(uint32_t baudRate);
        void Disconnect();
        void ScanSlaves();
        void ReadRegister(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
            uint32_t controlId, uint32_t commandId);
        void WriteRegister(uint8_t slaveAddr, uint8_t regAddr,
            const std::vector<uint8_t>& data,
            uint32_t controlId, uint32_t commandId);
        void SendCommand(uint8_t slaveAddr, uint8_t regAddr,
            uint32_t controlId, uint32_t commandId);
        void ReadAllRegisters(uint8_t defaultSlaveAddr, const std::vector<RegisterEntry>& entries);
        void ExecuteAllSingleTrigger(uint8_t defaultSlaveAddr, const std::vector<SingleTriggerEntry>& entries);

        void StartPeriodicExecution(uint8_t defaultSlaveAddr,
            const std::vector<PeriodicTriggerEntry>& entries,
            uint32_t intervalMs);
        void StopPeriodicExecution();

        void InsertSingleRead(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
            uint32_t controlId, uint32_t commandId);
        void InsertSingleWrite(uint8_t slaveAddr, uint8_t regAddr,
            const std::vector<uint8_t>& data,
            uint32_t controlId, uint32_t commandId);
        void InsertSingleCommand(uint8_t slaveAddr, uint8_t regAddr,
            uint32_t controlId, uint32_t commandId);

        void SetConnectCallback(ConnectCallback callback) { m_connectCallback = callback; }
        void SetDisconnectCallback(DisconnectCallback callback) { m_disconnectCallback = callback; }
        void SetScanCallback(ScanCallback callback) { m_scanCallback = callback; }
        void SetDataCallback(DataCallback callback) { m_dataCallback = callback; }
        void ProcessCallbacks();

        bool IsConnected() const { return m_isConnected; }
        bool IsPeriodicRunning() const { return m_periodicRunning; }

    private:
        void WorkerThread();
        void ProcessTask(const HardwareTask& task);
        void ExecutePeriodicTask();
        void ProcessPriorityTasks();
        void HandleDeviceDisconnected();
        ErrorType GetErrorType(int returnValue); std::thread m_workerThread;
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_isConnected{ false };
        std::atomic<bool> m_periodicRunning{ false };

        std::queue<HardwareTask> m_taskQueue;
        std::queue<HardwareTask> m_priorityQueue;
        std::mutex m_taskMutex;
        std::condition_variable m_taskCv;

        ConnectCallback m_connectCallback;
        DisconnectCallback m_disconnectCallback;
        ScanCallback m_scanCallback;
        DataCallback m_dataCallback;

        std::queue<std::function<void()>> m_callbackQueue;
        std::mutex m_callbackMutex;

        std::vector<PeriodicTriggerEntry> m_periodicEntries;
        uint8_t m_periodicSlaveAddr = 0;
        uint32_t m_periodicIntervalMs = 100;

        PMBus m_pmbus;
        std::mutex m_deviceMutex;
    };

}

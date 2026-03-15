#include "hardware_service.h"
#include <chrono>

namespace I2CDebugger {

    HardwareService::HardwareService() = default;

    HardwareService::~HardwareService() {
        Stop();
    }

    void HardwareService::Start() {
        m_running = true;
        m_workerThread = std::thread(&HardwareService::WorkerThread, this);
    }

    void HardwareService::Stop() {
        m_running = false;
        m_periodicRunning = false;
        m_taskCv.notify_all();

        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        m_pmbus.Close();
    }

    ErrorType HardwareService::GetErrorType(int returnValue) {
        if (returnValue >= 0) {
            return ErrorType::None;
        }
        else if (returnValue == SLAVE_NOT_RESPONSE) {
            return ErrorType::SlaveNotResponse;
        }
        else if (returnValue == DEVICE_NOT_CONNECTED) {
            return ErrorType::DeviceDisconnected;
        }
        else if (returnValue == CRC_ERROR_CODE) {
            return ErrorType::CRCNotCorrect;
        }
        return ErrorType::UnknownError;
    }

    void HardwareService::HandleDeviceDisconnected() {
        {
            std::lock_guard<std::mutex> lock(m_deviceMutex);
            m_pmbus.Close();
        }
        m_isConnected = false;
        m_periodicRunning = false;

        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            std::queue<HardwareTask> empty1;
            std::queue<HardwareTask> empty2;
            std::swap(m_taskQueue, empty1);
            std::swap(m_priorityQueue, empty2);
        }

        if (m_disconnectCallback) {
            std::lock_guard<std::mutex> cbLock(m_callbackMutex);
            m_callbackQueue.push([this]() {
                m_disconnectCallback();
                });
        }
    }

    void HardwareService::Connect(uint32_t baudRate) {
        HardwareTask task;
        task.type = TaskType::Connect;
        task.baudRate = baudRate;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::Disconnect() {
        HardwareTask task;
        task.type = TaskType::Disconnect;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::ScanSlaves() {
        HardwareTask task;
        task.type = TaskType::ScanSlaves;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    // --- 修改：添加 CRC 参数支持 ---
    void HardwareService::ReadRegister(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::ReadRegister;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.length = length;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::WriteRegister(uint8_t slaveAddr, uint8_t regAddr,
        const std::vector<uint8_t>& data,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::WriteRegister;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.data = data;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::SendCommand(uint8_t slaveAddr, uint8_t regAddr,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::SendCommand;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::ReadAllRegisters(uint8_t defaultSlaveAddr,
        const std::vector<RegisterEntry>& entries, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::ReadAllRegisters;
        task.slaveAddr = defaultSlaveAddr;
        task.registerEntries = entries;
        task.controlId = 1;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::ExecuteAllSingleTrigger(uint8_t defaultSlaveAddr,
        const std::vector<SingleTriggerEntry>& entries, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::ExecuteAllCommands;
        task.slaveAddr = defaultSlaveAddr;
        task.singleEntries = entries;
        task.controlId = 2;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::StartPeriodicExecution(uint8_t defaultSlaveAddr,
        const std::vector<PeriodicTriggerEntry>& entries,
        uint32_t intervalMs, bool crcEnabled, int crcType) {
        m_periodicEntries = entries;
        m_periodicSlaveAddr = defaultSlaveAddr;
        m_periodicIntervalMs = intervalMs;
        m_periodicCrcEnabled = crcEnabled;
        m_periodicCrcType = crcType;
        m_periodicRunning = true;
        m_taskCv.notify_one();
    }

    void HardwareService::StopPeriodicExecution() {
        m_periodicRunning = false;
    }

    void HardwareService::InsertSingleRead(uint8_t slaveAddr, uint8_t regAddr, uint8_t length,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::ReadRegister;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.length = length;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_priorityQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::InsertSingleWrite(uint8_t slaveAddr, uint8_t regAddr,
        const std::vector<uint8_t>& data,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::WriteRegister;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.data = data;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_priorityQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::InsertSingleCommand(uint8_t slaveAddr, uint8_t regAddr,
        uint32_t controlId, uint32_t commandId, bool crcEnabled, int crcType) {
        HardwareTask task;
        task.type = TaskType::SendCommand;
        task.slaveAddr = slaveAddr;
        task.regAddr = regAddr;
        task.controlId = controlId;
        task.commandId = commandId;
        task.crcEnabled = crcEnabled;
        task.crcType = crcType;
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_priorityQueue.push(task);
        m_taskCv.notify_one();
    }

    void HardwareService::ProcessCallbacks() {
        std::queue<std::function<void()>> callbacks;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            std::swap(callbacks, m_callbackQueue);
        }
        while (!callbacks.empty()) {
            callbacks.front()();
            callbacks.pop();
        }
    }

    void HardwareService::ProcessPriorityTasks() {
        while (m_isConnected) {
            HardwareTask task;
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                if (m_priorityQueue.empty()) break;
                task = m_priorityQueue.front();
                m_priorityQueue.pop();
            }
            ProcessTask(task);
        }
    }

    void HardwareService::WorkerThread() {
        while (m_running) {
            HardwareTask task;
            bool hasTask = false;

            {
                std::unique_lock<std::mutex> lock(m_taskMutex);
                if (!m_priorityQueue.empty()) {
                    task = m_priorityQueue.front();
                    m_priorityQueue.pop();
                    hasTask = true;
                }
                else if (!m_taskQueue.empty()) {
                    task = m_taskQueue.front();
                    m_taskQueue.pop();
                    hasTask = true;
                }
                else if (m_periodicRunning && m_isConnected) {
                    lock.unlock();
                    ExecutePeriodicTask();
                    continue;
                }
                else {
                    m_taskCv.wait_for(lock, std::chrono::milliseconds(10));
                    continue;
                }
            }

            if (hasTask) {
                ProcessTask(task);
            }
        }
    }

    void HardwareService::ProcessTask(const HardwareTask& task) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        switch (task.type) {
        case TaskType::Connect: {
            std::lock_guard<std::mutex> lock(m_deviceMutex);

            char* deviceName = nullptr;
            bool success = m_pmbus.Open(&deviceName);
            std::string devName = deviceName ? deviceName : "";
            std::string errorMsg;

            if (success) {
                success = m_pmbus.Configure(task.baudRate);
                if (!success) {
                    errorMsg = m_pmbus.GetLastError();
                    m_pmbus.Close();
                }
            }
            else {
                errorMsg = m_pmbus.GetLastError();
            }

            m_isConnected = success;
            if (m_connectCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this, success, devName, errorMsg]() {
                    m_connectCallback(success, devName, errorMsg);
                    });
            }
            break;
        }

        case TaskType::Disconnect: {
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                m_pmbus.Close();
            }
            m_isConnected = false;
            m_periodicRunning = false;

            if (m_connectCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this]() {
                    m_connectCallback(false, "", "");
                    });
            }
            break;
        }

        case TaskType::ScanSlaves: {
            std::vector<uint8_t> foundAddresses;
            int result;
            std::string errorMsg;

            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                result = m_pmbus.ScanDevices(0x02, 0x7F, foundAddresses);
                if (result < 0) {
                    errorMsg = m_pmbus.GetLastError();
                }
            }

            if (result == DEVICE_NOT_CONNECTED) {
                HandleDeviceDisconnected();
                if (m_scanCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, errorMsg]() {
                        m_scanCallback(false, std::vector<uint8_t>(), errorMsg);
                        });
                }
                break;
            }

            bool success = (result >= 0);

            if (m_scanCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this, success, foundAddresses, errorMsg]() {
                    m_scanCallback(success, foundAddresses, errorMsg);
                    });
            }
            break;
        }

        case TaskType::ReadRegister: {
            ResponsePacket packet;
            packet.controlId = task.controlId;
            packet.commandId = task.commandId;
            packet.timestamp = now;

            std::vector<uint8_t> result;
            int ret;
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：应用任务专属 CRC 配置

                ret = m_pmbus.Read(task.slaveAddr, task.regAddr, task.length, result);
                if (ret < 0) {
                    packet.errorMsg = m_pmbus.GetLastError();
                }
            }

            packet.success = (ret >= 0);
            packet.errorType = GetErrorType(ret);
            //if (packet.success) {
                packet.rawData = result;
            //}

            if (ret == DEVICE_NOT_CONNECTED) {
                if (m_dataCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, packet]() {
                        m_dataCallback(packet);
                        });
                }
                HandleDeviceDisconnected();
                break;
            }

            if (m_dataCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this, packet]() {
                    m_dataCallback(packet);
                    });
            }
            break;
        }

        case TaskType::WriteRegister: {
            ResponsePacket packet;
            packet.controlId = task.controlId;
            packet.commandId = task.commandId;
            packet.timestamp = now;

            int ret;
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：应用任务专属 CRC 配置

                ret = m_pmbus.Write(task.slaveAddr, task.regAddr, task.data);
                if (ret < 0) {
                    packet.errorMsg = m_pmbus.GetLastError();
                }
            }

            packet.success = (ret >= 0);
            packet.errorType = GetErrorType(ret);

            if (ret == DEVICE_NOT_CONNECTED) {
                if (m_dataCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, packet]() {
                        m_dataCallback(packet);
                        });
                }
                HandleDeviceDisconnected();
                break;
            }

            if (m_dataCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this, packet]() {
                    m_dataCallback(packet);
                    });
            }
            break;
        }

        case TaskType::SendCommand: {
            ResponsePacket packet;
            packet.controlId = task.controlId;
            packet.commandId = task.commandId;
            packet.timestamp = now;

            int ret;
            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：应用任务专属 CRC 配置

                ret = m_pmbus.SendByte(task.slaveAddr, task.regAddr);
                if (ret < 0) {
                    packet.errorMsg = m_pmbus.GetLastError();
                }
            }

            packet.success = (ret >= 0);
            packet.errorType = GetErrorType(ret);

            if (ret == DEVICE_NOT_CONNECTED) {
                if (m_dataCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, packet]() {
                        m_dataCallback(packet);
                        });
                }
                HandleDeviceDisconnected();
                break;
            }

            if (m_dataCallback) {
                std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                m_callbackQueue.push([this, packet]() {
                    m_dataCallback(packet);
                    });
            }
            break;
        }

        case TaskType::ReadAllRegisters: {
            if (m_useBatchMode) {
                std::vector<std::vector<uint8_t>> batchResults(task.registerEntries.size());
                std::vector<int> batchRet(task.registerEntries.size(), -1);
                bool flushSuccess = false;
                std::string batchError;

                {
                    std::lock_guard<std::mutex> lock(m_deviceMutex);
                    m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：同步批量任务 CRC 配置
                    m_pmbus.FlushOn();

                    for (size_t i = 0; i < task.registerEntries.size() && m_isConnected; i++) {
                        const auto& entry = task.registerEntries[i];
                        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : task.slaveAddr;
                        batchRet[i] = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, batchResults[i]);
                    }
                    flushSuccess = m_pmbus.Flush();
                    if (!flushSuccess) batchError = m_pmbus.GetLastError();
                    m_pmbus.FlushOff();
                }

                // 统一分发回调
                for (size_t i = 0; i < task.registerEntries.size() && m_isConnected; i++) {
                    ResponsePacket packet;
                    packet.controlId = 1;
                    packet.commandId = static_cast<uint32_t>(i);
                    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

                    if (batchResults[i].size() == task.registerEntries[i].length) {
                        packet.success = true;
                        packet.rawData = batchResults[i];
                        packet.errorType = ErrorType::None;
                    }
                    else {
                        packet.success = false;
                        packet.errorType = ErrorType::SlaveNotResponse;
                        packet.errorMsg = batchError.empty() ? "Batch Read Failed" : batchError;
                    }

                    if (m_dataCallback) {
                        std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                        m_callbackQueue.push([this, packet]() { m_dataCallback(packet); });
                    }
                }
            }
            else {
                for (size_t i = 0; i < task.registerEntries.size() && m_isConnected; i++) {
                    ProcessPriorityTasks();
                    if (!m_isConnected) break;

                    const auto& entry = task.registerEntries[i];
                    uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : task.slaveAddr;
                    ResponsePacket packet;
                    packet.controlId = 1;
                    packet.commandId = static_cast<uint32_t>(i);
                    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

                    std::vector<uint8_t> result;
                    int ret;
                    {
                        std::lock_guard<std::mutex> lock(m_deviceMutex);
                        m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：应用任务专属 CRC 配置
                        ret = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, result);
                        if (ret < 0) {
                            packet.errorMsg = m_pmbus.GetLastError();
                        }
                    }

                    packet.success = (ret >= 0);
                    packet.errorType = GetErrorType(ret);

                    if (packet.success) {
                        packet.rawData = result;
                    }

                    if (m_dataCallback) {
                        std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                        m_callbackQueue.push([this, packet]() {
                            m_dataCallback(packet);
                            });
                    }

                    if (ret == DEVICE_NOT_CONNECTED) {
                        HandleDeviceDisconnected();
                        break;
                    }
                }
            }

            break;
        }

        case TaskType::ExecuteAllCommands: {
            if (m_useBatchMode) {
                std::vector<std::vector<uint8_t>> batchResults(task.singleEntries.size());
                std::vector<int> batchRet(task.singleEntries.size(), -1);
                bool flushSuccess = false;
                std::string batchError;

                {
                    std::lock_guard<std::mutex> lock(m_deviceMutex);
                    m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：同步批量任务 CRC 配置
                    m_pmbus.FlushOn();

                    for (size_t i = 0; i < task.singleEntries.size() && m_isConnected; i++) {
                        const auto& entry = task.singleEntries[i];
                        if (!entry.enabled) continue;

                        uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : task.slaveAddr;
                        switch (entry.type) {
                        case CommandType::Read:
                            batchRet[i] = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, batchResults[i]);
                            break;
                        case CommandType::Write:
                            batchRet[i] = m_pmbus.Write(slaveAddr, entry.regAddress, entry.data);
                            break;
                        case CommandType::SendCommand:
                            batchRet[i] = m_pmbus.SendByte(slaveAddr, entry.regAddress);
                            break;
                        }

                        if (entry.delayMs > 0) {
                            m_pmbus.Flush();
                            std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
                        }
                    }
                    flushSuccess = m_pmbus.Flush();
                    if (!flushSuccess) batchError = m_pmbus.GetLastError();
                    m_pmbus.FlushOff();
                }

                for (size_t i = 0; i < task.singleEntries.size() && m_isConnected; i++) {
                    const auto& entry = task.singleEntries[i];
                    if (!entry.enabled) continue;

                    ResponsePacket packet;
                    packet.controlId = 2;
                    packet.commandId = static_cast<uint32_t>(i);
                    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

                    if (entry.type == CommandType::Read) {
                        if (batchResults[i].size() == entry.length) {
                            packet.success = true;
                            packet.rawData = batchResults[i];
                        }
                        else {
                            packet.success = false;
                            packet.errorMsg = batchError.empty() ? "Batch Read Mismatch" : batchError;
                        }
                    }
                    else {
                        packet.success = flushSuccess;
                        if (!flushSuccess) packet.errorMsg = batchError;
                    }

                    if (m_dataCallback) {
                        std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                        m_callbackQueue.push([this, packet]() { m_dataCallback(packet); });
                    }
                }
            }
            else {
                for (size_t i = 0; i < task.singleEntries.size() && m_isConnected; i++) {
                    ProcessPriorityTasks();
                    if (!m_isConnected) break;

                    const auto& entry = task.singleEntries[i];
                    if (!entry.enabled) continue;

                    uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : task.slaveAddr;

                    ResponsePacket packet;
                    packet.controlId = 2;
                    packet.commandId = static_cast<uint32_t>(i);
                    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

                    int ret = 0;

                    {
                        std::lock_guard<std::mutex> lock(m_deviceMutex);
                        m_pmbus.SetCrcConfig(task.crcEnabled, task.crcType); // --- 修改：应用任务专属 CRC 配置

                        switch (entry.type) {
                        case CommandType::Read: {
                            std::vector<uint8_t> result;
                            ret = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, result);
                            if (ret >= 0) {
                                packet.rawData = result;
                            }
                            break;
                        }
                        case CommandType::Write:ret = m_pmbus.Write(slaveAddr, entry.regAddress, entry.data);
                            break;
                        case CommandType::SendCommand:
                            ret = m_pmbus.SendByte(slaveAddr, entry.regAddress);
                            break;
                        }

                        if (ret < 0) {
                            packet.errorMsg = m_pmbus.GetLastError();
                        }
                    }

                    packet.success = (ret >= 0);
                    packet.errorType = GetErrorType(ret);

                    if (m_dataCallback) {
                        std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                        m_callbackQueue.push([this, packet]() {
                            m_dataCallback(packet);
                            });
                    }

                    if (ret == DEVICE_NOT_CONNECTED) {
                        HandleDeviceDisconnected();
                        break;
                    }

                    if (entry.delayMs > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
                    }
                }
            }
            break;
        }

        default:
            break;
        }
    }

    void HardwareService::ExecutePeriodicTask() {
        if (!m_isConnected || !m_periodicRunning) return;

        auto startTime = std::chrono::steady_clock::now();

        if (m_useBatchMode) {
            std::vector<std::vector<uint8_t>> batchResults(m_periodicEntries.size());
            std::vector<int> batchRet(m_periodicEntries.size(), -1);
            bool flushSuccess = false;
            std::string batchError;

            {
                std::lock_guard<std::mutex> lock(m_deviceMutex);
                m_pmbus.SetCrcConfig(m_periodicCrcEnabled, m_periodicCrcType); // --- 修改：应用周期任务 CRC 配置
                m_pmbus.FlushOn();

                for (size_t i = 0; i < m_periodicEntries.size() && m_periodicRunning && m_isConnected; i++) {
                    const auto& entry = m_periodicEntries[i];
                    if (!entry.enabled) continue;

                    uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : m_periodicSlaveAddr;
                    switch (entry.type) {
                    case CommandType::Read:
                        batchRet[i] = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, batchResults[i]);
                        break;
                    case CommandType::Write:
                        batchRet[i] = m_pmbus.Write(slaveAddr, entry.regAddress, entry.data);
                        break;
                    case CommandType::SendCommand:
                        batchRet[i] = m_pmbus.SendByte(slaveAddr, entry.regAddress);
                        break;
                    }

                    if (entry.delayMs > 0) {
                        m_pmbus.Flush();
                        std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
                    }
                }
                flushSuccess = m_pmbus.Flush();
                if (!flushSuccess) batchError = m_pmbus.GetLastError();
                m_pmbus.FlushOff();
            }

            for (size_t i = 0; i < m_periodicEntries.size() && m_periodicRunning && m_isConnected; i++) {
                const auto& entry = m_periodicEntries[i];
                if (!entry.enabled) continue;

                ResponsePacket packet;
                packet.controlId = 3;
                packet.commandId = static_cast<uint32_t>(i);
                packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                if (entry.type == CommandType::Read) {
                    if (batchResults[i].size() == entry.length) {
                        packet.success = true;
                        packet.rawData = batchResults[i];
                    }
                    else {
                        packet.success = false;
                        packet.errorMsg = batchError.empty() ? "Batch NACK" : batchError;
                    }
                }
                else {
                    packet.success = flushSuccess;
                    if (!flushSuccess) packet.errorMsg = batchError;
                }

                if (m_dataCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, packet]() { m_dataCallback(packet); });
                }
            }
        }
        else {
            for (size_t i = 0; i < m_periodicEntries.size() && m_periodicRunning && m_isConnected; i++) {
                ProcessPriorityTasks();
                if (!m_isConnected || !m_periodicRunning) break;

                const auto& entry = m_periodicEntries[i];
                if (!entry.enabled) continue;

                uint8_t slaveAddr = entry.overrideSlaveAddr ? entry.slaveAddress : m_periodicSlaveAddr;

                ResponsePacket packet;
                packet.controlId = 3;
                packet.commandId = static_cast<uint32_t>(i);
                packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                int ret = 0;

                {
                    std::lock_guard<std::mutex> lock(m_deviceMutex);
                    m_pmbus.SetCrcConfig(m_periodicCrcEnabled, m_periodicCrcType); // --- 修改：应用周期任务 CRC 配置

                    switch (entry.type) {
                    case CommandType::Read: {
                        std::vector<uint8_t> result;
                        ret = m_pmbus.Read(slaveAddr, entry.regAddress, entry.length, result);
                        if (ret >= 0) {
                            packet.rawData = result;
                        }
                        break;
                    }
                    case CommandType::Write:
                        ret = m_pmbus.Write(slaveAddr, entry.regAddress, entry.data);
                        break;
                    case CommandType::SendCommand:
                        ret = m_pmbus.SendByte(slaveAddr, entry.regAddress);
                        break;
                    }

                    if (ret < 0) {
                        packet.errorMsg = m_pmbus.GetLastError();
                    }
                }

                packet.success = (ret >= 0);
                packet.errorType = GetErrorType(ret);

                if (m_dataCallback) {
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_callbackQueue.push([this, packet]() {
                        m_dataCallback(packet);
                        });
                }

                if (ret == DEVICE_NOT_CONNECTED) {
                    HandleDeviceDisconnected();
                    return;
                }

                if (entry.delayMs > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
                }
            }
        }

        if (m_periodicRunning && m_isConnected) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            auto remaining = std::chrono::milliseconds(m_periodicIntervalMs) - elapsed;
            if (remaining.count() > 0) {
                std::this_thread::sleep_for(remaining);
            }
        }
    }

    void HardwareService::EnableBatchMode(bool enable) {
        m_useBatchMode = enable;
    }

}

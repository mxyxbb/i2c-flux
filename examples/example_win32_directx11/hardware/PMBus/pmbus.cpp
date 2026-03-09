// pmbus.cpp

#include "pmbus.h"
#include <cstring>
#include <stdexcept>
#include <algorithm> 
#include <cctype>    
#include <thread>
#include <chrono>

namespace {
    int extractPortNumber(const std::string& portName) {
        int num = 0;
        int multiplier = 1;
        for (int i = portName.length() - 1; i >= 0; --i) {
            if (std::isdigit(portName[i])) {
                num += (portName[i] - '0') * multiplier;
                multiplier *= 10;
            }
            else if (multiplier > 1) {
                break;
            }
        }
        return num;
    }

    bool containsIgnoreCase(const std::string& str, const std::string& sub) {
        auto it = std::search(
            str.begin(), str.end(),
            sub.begin(), sub.end(),
            [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
        );
        return (it != str.end());
    }
}

PMBus::PMBus() = default;

PMBus::~PMBus() {
    Close();
}

bool PMBus::executeSerialCommand(const std::vector<uint8_t>& tx_data, RPI2C::Packet& rx_packet, int timeout_ms) {
    protocolParser_.reset();
    serialPort_.write(tx_data);

    auto start_time = std::chrono::steady_clock::now();

    // 在设定的超时时间内不断轮询
    while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
        size_t available = serialPort_.available(); // 查询当前系统缓冲区有多少数据
        if (available > 0) {
            std::vector<uint8_t> rx_buffer;
            serialPort_.read(rx_buffer, available); // 请求确切的字节数，瞬间返回不阻塞

            for (uint8_t byte : rx_buffer) {
                if (protocolParser_.parseByte(byte, rx_packet)) {
                    return true; // 成功解析到一帧数据，立即退出！无任何多余 Delay
                }
            }
        }
        else {
            // 缓冲区无数据，休眠 1ms 避免 CPU 占用 100%
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return false; // 真正超时才返回失败
}

bool PMBus::Open(char** deviceName) {
    if (currentMode_ != DeviceMode::MODE_NONE) Close();
    FlushOff(); // 确保打开设备时状态被重置

    int ret = SMBus_Open(&device_, deviceName);
    if (ret == 0) {
        currentMode_ = DeviceMode::MODE_SMBUS;
        return true;
    }

    lastError_ = "SMBus_Open failed with code: " + std::to_string(ret) + ". Attempting Serial scan...";

    std::vector<serial::PortInfo> ports = serial::list_ports();
    if (ports.empty()) return false;

    std::sort(ports.begin(), ports.end(), [](const serial::PortInfo& a, const serial::PortInfo& b) {
        return extractPortNumber(a.port) > extractPortNumber(b.port);
        });

    std::vector<uint8_t> tx_data = RPI2C::Protocol::packGetSignature();
    RPI2C::Packet rx_packet;

    for (const auto& port_info : ports) {
        if (containsIgnoreCase(port_info.description, "bluetooth") ||
            containsIgnoreCase(port_info.description, "蓝牙") ||
            containsIgnoreCase(port_info.hardware_id, "bthenum")) {
            continue;
        }

        try {
            serialPort_.setPort(port_info.port);
            serialPort_.setBaudrate(115200);
            serialPort_.setTimeout(serial::Timeout::simpleTimeout(100));
            serialPort_.open();

            if (serialPort_.isOpen()) {
                serialPort_.setDTR(true);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        catch (...) { continue; }

        if (!serialPort_.isOpen()) continue;

        if (executeSerialCommand(tx_data, rx_packet, 100)) {
            if (rx_packet.cmd == (RPI2C::CMD_GET_SIG | 0x80)) {
                currentMode_ = DeviceMode::MODE_SERIAL;
                if (deviceName) {
                    // 利用 std::string 方便地进行安全拼接
                    std::string fullName = std::string(DEV_RPI2C_HEADER_NAME) + "_" + port_info.port;

                    // strdup 会在堆上重新分配内存并拷贝 fullName 的内容，
                    // 所以即使 fullName 在 if 块结束后销毁，传出的指针依然有效。
                    *deviceName = strdup(fullName.c_str());
                }
                return true;
            }
        }
        serialPort_.close();
    }

    lastError_ = "Failed to open SMBus adapter.";
    return false;
}

void PMBus::Close() {
    FlushOff(); // 关闭时清空所有未发出的缓冲区
    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        SMBus_Close(device_);
    }
    else if (currentMode_ == DeviceMode::MODE_SERIAL) {
        if (serialPort_.isOpen()) {
            serialPort_.close();
        }
    }
    currentMode_ = DeviceMode::MODE_NONE;
}

bool PMBus::Configure(uint32_t bitrate) {
    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        int ret = SMBus_Configure(
            device_, static_cast<DWORD>(bitrate), DEFAULT_SLAVE_ADDR,
            DEFAULT_AUTO_READ_RESPOND, DEFAULT_WRITE_TIMEOUT,
            DEFAULT_READ_TIMEOUT, DEFAULT_SCL_LOW_TIMEOUT,
            DEFAULT_TRANSFER_RETRIES, DEFAULT_RESPONSE_TIMEOUT
        );
        if (ret != 0) {
            lastError_ = "SMBus_Configure failed: " + std::to_string(ret);
            return false;
        }
        return true;
    }
    else if (currentMode_ == DeviceMode::MODE_SERIAL) {
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packSetBaudrate(bitrate);
        RPI2C::Packet rx_packet;

        // 配置指令由于关乎底层时钟，最好立刻执行，不进 Flush 缓冲
        if (executeSerialCommand(tx_data, rx_packet)) {
            return (rx_packet.payload[0] == 0x00);
        }
        lastError_ = "Serial configuration timeout.";
        return false;
    }

    lastError_ = "Device not open";
    return false;
}

// --- Flush 控制接口 ---
void PMBus::FlushOn() {
    flushMode_ = true;
    txBuffer_.clear();
    commandQueue_.clear();
}

void PMBus::FlushOff() {
    flushMode_ = false;
    txBuffer_.clear();
    commandQueue_.clear();
}

bool PMBus::Flush() {
    if (!flushMode_ || currentMode_ != DeviceMode::MODE_SERIAL) return false;
    if (txBuffer_.empty()) return true;

    protocolParser_.reset();
    serialPort_.write(txBuffer_);

    int receivedPackets = 0;
    int expectedPackets = commandQueue_.size();
    bool allSuccess = true;

    auto start_time = std::chrono::steady_clock::now();

    RPI2C::Packet rx_packet;

    // 批处理允许稍长的总超时时间（例如给 500ms）
    while (receivedPackets < expectedPackets &&
        std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(500)) {

        size_t available = serialPort_.available();
        if (available > 0) {
            std::vector<uint8_t> rx_buffer;
            serialPort_.read(rx_buffer, available); // 瞬间返回

            for (uint8_t byte : rx_buffer) {
                if (protocolParser_.parseByte(byte, rx_packet)) {
                    if (receivedPackets < expectedPackets) {
                        auto& cmd = commandQueue_[receivedPackets];
                        if (cmd.type == PendingTask::READ && cmd.readResult != nullptr) {
                            if (rx_packet.len == cmd.readLen) {
                                *(cmd.readResult) = rx_packet.payload;
                            }
                            else {
                                allSuccess = false;
                                lastError_ = "Batch Read length mismatch.";
                            }
                        }
                        else {
                            if (rx_packet.payload.empty() || rx_packet.payload[0] != 0x00) {
                                allSuccess = false;
                                lastError_ = "Batch Write NACK.";
                            }
                        }
                        receivedPackets++;
                    }
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (receivedPackets < expectedPackets) {
        allSuccess = false;
        lastError_ = "Batch flush timeout. Expected " + std::to_string(expectedPackets) +
            " but got " + std::to_string(receivedPackets);
    }

    txBuffer_.clear();
    commandQueue_.clear();
    return allSuccess;
}

// --- 重写通信接口 ---

INT PMBus::Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        // SMBus 不支持 Flush，照常直发
        std::vector<uint8_t> buffer;
        buffer.push_back(regAddr);
        buffer.insert(buffer.end(), data.begin(), data.end());

        int ret = SMBus_Write(device_, buffer.data(), slaveAddress << 1, static_cast<BYTE>(buffer.size()));
        if (ret != 0) lastError_ = "SMBus_Write failed: " + std::to_string(ret);
        return ret;
    }
    else {
        std::vector<uint8_t> payload = { regAddr };
        payload.insert(payload.end(), data.begin(), data.end());
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWrite(slaveAddress, payload);

        // --- 命中 Flush 机制 ---
        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            commandQueue_.push_back({ PendingTask::WRITE, nullptr, 0 });
            return 0; // 返回成功，表示已加入队列
        }

        RPI2C::Packet rx_packet;
        if (executeSerialCommand(tx_data, rx_packet)) {
            if (!rx_packet.payload.empty() && rx_packet.payload[0] == 0x00) return 0;
            lastError_ = "Serial I2C Write NACK.";
            return SLAVE_NOT_RESPONSE;
        }
        return DEVICE_NOT_CONNECTED;
    }
}

INT PMBus::Read(uint8_t slaveAddress, uint8_t regAddr, uint16_t numBytes, std::vector<uint8_t>& result) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    result.clear();

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        result.resize(numBytes);
        int ret = SMBus_WriteRead(device_, result.data(), slaveAddress << 1, static_cast<WORD>(numBytes), static_cast<BYTE>(1), &regAddr);
        if (ret < 0) lastError_ = "SMBus_WriteRead failed: " + std::to_string(ret);
        return ret;
    }
    else {
        std::vector<uint8_t> target_reg = { regAddr };
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWriteRead(slaveAddress, numBytes, target_reg);

        // --- 命中 Flush 机制 ---
        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            // 传入 result 的地址，等 Flush() 收到数据后再解引用写进去
            commandQueue_.push_back({ PendingTask::READ, &result, numBytes });
            return 0;
        }

        RPI2C::Packet rx_packet;
        if (executeSerialCommand(tx_data, rx_packet)) {
            if (rx_packet.len == numBytes) {
                result = rx_packet.payload;
                return 0;
            }
            else {
                lastError_ = "Serial I2C Read NACK.";
                return SLAVE_NOT_RESPONSE;
            }
        }
        return DEVICE_NOT_CONNECTED;
    }
}

INT PMBus::SendByte(uint8_t slaveAddress, uint8_t byte) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        int ret = SMBus_Write(device_, &byte, slaveAddress << 1, static_cast<BYTE>(1));
        if (ret != 0) lastError_ = "SMBus_Write_Byte failed: " + std::to_string(ret);
        return ret;
    }
    else {
        std::vector<uint8_t> payload = { byte };
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWrite(slaveAddress, payload);

        // --- 命中 Flush 机制 ---
        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            commandQueue_.push_back({ PendingTask::WRITE, nullptr, 0 });
            return 0;
        }

        RPI2C::Packet rx_packet;
        if (executeSerialCommand(tx_data, rx_packet)) {
            if (!rx_packet.payload.empty() && rx_packet.payload[0] == 0x00) return 0;
            return SLAVE_NOT_RESPONSE;
        }
        return DEVICE_NOT_CONNECTED;
    }
}

INT PMBus::ScanDevices(uint8_t startAddr, uint8_t endAddr, std::vector<uint8_t>& foundAddresses) {
    if (currentMode_ == DeviceMode::MODE_NONE) return -1;
    if (startAddr > 0x7F || endAddr > 0x7F) {
        lastError_ = "Invalid slave address";
        return -1;
    }

    foundAddresses.clear();

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        std::vector<BYTE> addrGroup(128, 0);
        int ret = SMBus_Scan(device_, addrGroup.data(), startAddr, endAddr);
        if (ret < 0) {
            lastError_ = "SMBus_Scan failed: " + std::to_string(ret);
            return ret;
        }
        for (int i = 0; i <= endAddr - startAddr; ++i) {
            if (addrGroup[i] != 0) foundAddresses.push_back(static_cast<uint8_t>(addrGroup[i]));
        }
        return ret;
    }
    else {
        // --- RP2040 Serial I2C Scan: 使用 Flush 模式和 packRead 进行批量探测 ---

        bool prevFlush = flushMode_; // 保存用户原有的 Flush 状态
        FlushOn();                   // 强制开启缓冲

        // 为每个探测地址预留一个接收容器
        std::vector<std::vector<uint8_t>> scanResults(endAddr - startAddr + 1);

        for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
            // 使用 packRead 封包，尝试纯读 1 个字节
            std::vector<uint8_t> tx_data = RPI2C::Protocol::packRead(addr, 1);

            // 填入统一发送缓冲区
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            // 绑定对应的接收容器
            commandQueue_.push_back({ PendingTask::READ, &scanResults[addr - startAddr], 1 });
        }

        // 一次性发出所有探测指令并等待 RP2040 批量回复
        Flush();

        // 分析解包后的结果
        for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
            // 如果某地址对应的容器被填入了 1 个字节，说明 I2C 读操作成功 (收到 ACK 并回传数据)
            if (scanResults[addr - startAddr].size() == 1) {
                foundAddresses.push_back(addr);
            }
        }

        // 扫描完成，恢复调用此函数之前的状态
        if (!prevFlush) {
            FlushOff();
        }

        return 0;
    }
}

std::string PMBus::GetLastError() const {
    return lastError_;
}

// pmbus.cpp

#include "pmbus.h"
#include <cstring>
#include <stdexcept>
#include <algorithm> 
#include <cctype>    
#include <thread>
#include <chrono>
#include "core/models/crc_calculator.h"

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

    int GetCrcByteLength(int type) {
        if (type == 1) return 2; // CRC-16 (Modbus)
        if (type == 2) return 4; // CRC-32
        return 1;                // CRC-8 (SMBus)
    }

    // 辅助函数：将多字节 CRC 采用小端序追加到数据流尾部
    void AppendCrcToData(std::vector<uint8_t>& data, uint32_t crc, int type) {
        int len = GetCrcByteLength(type);
        for (int i = 0; i < len; ++i) {
            data.push_back(static_cast<uint8_t>((crc >> (i * 8)) & 0xFF));
        }
    }

    // 辅助函数：校验接收到的数据包 CRC 是否正确
    bool VerifyPacketCrc(const std::vector<uint8_t>& packet, const std::vector<uint8_t>& header, int type) {
        int crcLen = GetCrcByteLength(type);
        if (packet.size() < crcLen) return false;

        // 提取前置数据
        std::vector<uint8_t> crc_payload = header;
        crc_payload.insert(crc_payload.end(), packet.begin(), packet.end() - crcLen);

        // 调用静态算法计算预期值
        uint32_t calc_crc = I2CDebugger::CrcCalculator::Calculate(type, crc_payload);

        // 提取接收到的 CRC（小端序恢复）
        uint32_t rx_crc = 0;
        for (int i = 0; i < crcLen; ++i) {
            rx_crc |= (static_cast<uint32_t>(packet[packet.size() - crcLen + i]) << (i * 8));
        }

        return calc_crc == rx_crc;
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

    while (receivedPackets < expectedPackets &&
        std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(3000)) {

        size_t available = serialPort_.available();
        if (available > 0) {
            std::vector<uint8_t> rx_buffer;
            serialPort_.read(rx_buffer, available);

            for (uint8_t byte : rx_buffer) {
                if (protocolParser_.parseByte(byte, rx_packet)) {
                    if (receivedPackets < expectedPackets) {
                        auto& cmd = commandQueue_[receivedPackets];
                        if (cmd.type == PendingTask::READ && cmd.readResult != nullptr) {
                            if (rx_packet.len == cmd.readLen) {
                                // --- 【修改】批量模式下的动态 CRC 校验 ---
                                if (cmd.useCrc) {
                                    std::vector<uint8_t> header = {
                                        static_cast<uint8_t>(cmd.slaveAddress << 1),       // Write Addr
                                        cmd.regAddr,                                       // Register
                                        static_cast<uint8_t>((cmd.slaveAddress << 1) | 1)  // Read Addr
                                    };

                                    bool crcOk = VerifyPacketCrc(rx_packet.payload, header, cmd.crcType);
                                    int crcLen = GetCrcByteLength(cmd.crcType);

                                    // 无论CRC对错，先把尾部的 CRC 字节剥离出去并赋值给外部
                                    if (rx_packet.payload.size() >= static_cast<size_t>(crcLen)) {
                                        rx_packet.payload.erase(rx_packet.payload.end() - crcLen, rx_packet.payload.end());
                                    }
                                    *(cmd.readResult) = rx_packet.payload;

                                    // 然后再报错误
                                    if (!crcOk) {
                                        allSuccess = false;
                                        lastError_ = "Batch Read CRC Error.";
                                    }
                                }
                                else {
                                    *(cmd.readResult) = rx_packet.payload;
                                }
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
        lastError_ = "Batch flush timeout.";
    }

    txBuffer_.clear();
    commandQueue_.clear();
    return allSuccess;
}

INT PMBus::Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    std::vector<uint8_t> actual_data = data;

    // --- 使能CRC后，动态追加 1/2/4 字节 ---
    if (crcEnabled_) {
        std::vector<uint8_t> crc_payload = { static_cast<uint8_t>(slaveAddress << 1), regAddr };
        crc_payload.insert(crc_payload.end(), data.begin(), data.end());
        uint32_t calc_crc = I2CDebugger::CrcCalculator::Calculate(crcType_, crc_payload);
        AppendCrcToData(actual_data, calc_crc, crcType_);
    }

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        std::vector<uint8_t> buffer;
        buffer.push_back(regAddr);
        buffer.insert(buffer.end(), actual_data.begin(), actual_data.end());

        int ret = SMBus_Write(device_, buffer.data(), slaveAddress << 1, static_cast<BYTE>(buffer.size()));
        if (ret != 0) lastError_ = "SMBus_Write failed: " + std::to_string(ret);
        return ret;
    }
    else {
        std::vector<uint8_t> payload = { regAddr };
        payload.insert(payload.end(), actual_data.begin(), actual_data.end());
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWrite(slaveAddress, payload);

        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            // 记录当前的 crcType_ 以备 Flush 校验用
            commandQueue_.push_back({ PendingTask::WRITE, nullptr, 0, slaveAddress, regAddr, crcEnabled_, crcType_ });
            return 0;
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

    // --- 读取时请求 长度 + (CRC字节数) ---
    int crcLen = GetCrcByteLength(crcType_);
    uint16_t actual_numBytes = crcEnabled_ ? numBytes + crcLen : numBytes;

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        std::vector<uint8_t> temp_result(actual_numBytes);
        int ret = SMBus_WriteRead(device_, temp_result.data(), slaveAddress << 1, static_cast<WORD>(actual_numBytes), static_cast<BYTE>(1), &regAddr);
        if (ret < 0) {
            lastError_ = "SMBus_WriteRead failed: " + std::to_string(ret);
            return ret;
        }

        if (crcEnabled_) {
            std::vector<uint8_t> header = {
                static_cast<uint8_t>((slaveAddress << 1) | 1),
                regAddr
            };

            bool crcOk = VerifyPacketCrc(temp_result, header, crcType_);

            // 【修改】无论CRC对错，先把尾部的 CRC 字节剥离，把数据保留下来
            if (temp_result.size() >= static_cast<size_t>(crcLen)) {
                temp_result.erase(temp_result.end() - crcLen, temp_result.end());
            }
            result = temp_result; // 传回剥离CRC后的真实数据

            // 然后再报错
            if (!crcOk) {
                lastError_ = "SMBus Read CRC Error.";
                return CRC_ERROR_CODE;
            }
        }
        else {
            result = temp_result;
        }
        return ret;
    }
    else {
        std::vector<uint8_t> target_reg = { regAddr };
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWriteRead(slaveAddress, actual_numBytes, target_reg);

        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            commandQueue_.push_back({ PendingTask::READ, &result, actual_numBytes, slaveAddress, regAddr, crcEnabled_, crcType_ });
            return 0;
        }

        RPI2C::Packet rx_packet;
        if (executeSerialCommand(tx_data, rx_packet)) {
            if (rx_packet.len == actual_numBytes) {
                if (crcEnabled_) {
                    std::vector<uint8_t> header = {
                        static_cast<uint8_t>((slaveAddress << 1) | 1),
                        regAddr
                    };

                    bool crcOk = VerifyPacketCrc(rx_packet.payload, header, crcType_);

                    // 【修改】无论CRC对错，剥离尾部 CRC 字节，把数据保留下来
                    if (rx_packet.payload.size() >= static_cast<size_t>(crcLen)) {
                        rx_packet.payload.erase(rx_packet.payload.end() - crcLen, rx_packet.payload.end());
                    }
                    result = rx_packet.payload; // 传回剥离CRC后的真实数据

                    if (!crcOk) {
                        lastError_ = "Serial I2C Read CRC Error.";
                        return CRC_ERROR_CODE;
                    }
                }
                else {
                    result = rx_packet.payload;
                }
                return 0;
            }
            else {
                lastError_ = "Serial I2C Read NACK or Length mismatch.";
                return SLAVE_NOT_RESPONSE;
            }
        }
        return DEVICE_NOT_CONNECTED;
    }
}

INT PMBus::SendByte(uint8_t slaveAddress, uint8_t byte) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    std::vector<uint8_t> actual_data = { byte };

    if (crcEnabled_) {
        std::vector<uint8_t> crc_payload = { static_cast<uint8_t>(slaveAddress << 1), byte };
        uint32_t calc_crc = I2CDebugger::CrcCalculator::Calculate(crcType_, crc_payload);
        AppendCrcToData(actual_data, calc_crc, crcType_);
    }

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        int ret = SMBus_Write(device_, actual_data.data(), slaveAddress << 1, static_cast<BYTE>(actual_data.size()));
        if (ret != 0) lastError_ = "SMBus_Write failed (SendByte): " + std::to_string(ret);
        return ret;
    }
    else {
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWrite(slaveAddress, actual_data);

        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            commandQueue_.push_back({ PendingTask::WRITE, nullptr, 0, slaveAddress, byte, crcEnabled_, crcType_ });
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


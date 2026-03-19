// pmbus.cpp

#include "pmbus.h"
#include <cstring>
#include <stdexcept>
#include <algorithm> 
#include <cctype>    
#include <thread>
#include <chrono>
#include "core/models/crc_calculator.h"

#define CRC_ERROR_CODE -3 

namespace {
    int extractPortNumber(const std::string& portName) {
        int num = 0;
        int multiplier = 1;
        for (int i = portName.length() - 1; i >= 0; --i) {
            if (std::isdigit(portName[i])) {
                num += (portName[i] - '0') * multiplier;
                multiplier *= 10;
            }
            else if (multiplier > 1) break;
        }
        return num;
    }

    int GetCrcByteLength(int type) {
        if (type == 1) return 2; // CRC-16
        if (type == 2) return 4; // CRC-32
        return 1;                // CRC-8
    }

    void AppendCrcToData(std::vector<uint8_t>& data, uint32_t crc, int type) {
        int len = GetCrcByteLength(type);
        for (int i = 0; i < len; ++i) {
            data.push_back(static_cast<uint8_t>((crc >> (i * 8)) & 0xFF));
        }
    }

    bool VerifyPacketCrc(const std::vector<uint8_t>& packet, const std::vector<uint8_t>& header, int type) {
        int crcLen = GetCrcByteLength(type);
        if (packet.size() < crcLen) return false;

        std::vector<uint8_t> crc_payload = header;
        crc_payload.insert(crc_payload.end(), packet.begin(), packet.end() - crcLen);

        uint32_t calc_crc = I2CDebugger::CrcCalculator::Calculate(type, crc_payload);

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

PMBus::~PMBus() { Close(); }

bool PMBus::executeSerialCommand(const std::vector<uint8_t>& tx_data, RPI2C::Packet& rx_packet, int timeout_ms) {
    protocolParser_.reset();
    try { serialPort_.write(tx_data); }
    catch (const std::exception& e) {
        lastError_ = std::string("Serial write exception: ") + e.what();
        return false;
    }

    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
        try {
            size_t available = serialPort_.available();
            if (available > 0) {
                std::vector<uint8_t> rx_buffer;
                serialPort_.read(rx_buffer, available);

                for (uint8_t byte : rx_buffer) {
                    if (protocolParser_.parseByte(byte, rx_packet)) return true;
                }
            }
            else { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        }
        catch (const std::exception& e) {
            lastError_ = std::string("Serial read exception: ") + e.what();
            return false;
        }
    }
    lastError_ = "Serial command timeout.";
    return false;
}

bool PMBus::Open(char** deviceName) {
    if (currentMode_ != DeviceMode::MODE_NONE) Close();
    FlushOff();

    // --- 1. 优先尝试扫描打开 CH347T 设备 ---
    HANDLE hCH347 = CH347OpenDevice(ch347DeviceIndex_);
    if (hCH347 != INVALID_HANDLE_VALUE && hCH347 != nullptr) {
        // 根据 Demo 补充超时配置
        CH347SetTimeout(ch347DeviceIndex_, 500, 500);
        currentMode_ = DeviceMode::MODE_CH347T;
        if (deviceName) {
            *deviceName = strdup("CH347T_I2C_Adapter");
        }
        return true;
    }

    // --- 2. 尝试 SMBus ---
    int ret = SMBus_Open(&device_, deviceName);
    if (ret == 0) {
        currentMode_ = DeviceMode::MODE_SMBUS;
        return true;
    }

    // --- 3. 尝试 Serial 扫描 ---
    lastError_ = "SMBus/CH347 Open failed. Attempting Serial scan...";
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
                    std::string fullName = std::string(DEV_RPI2C_HEADER_NAME) + "_" + port_info.port;
                    *deviceName = strdup(fullName.c_str());
                }
                return true;
            }
        }
        serialPort_.close();
    }

    lastError_ = "Failed to open any I2C adapter.";
    return false;
}

void PMBus::Close() {
    FlushOff();
    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        SMBus_Close(device_);
    }
    else if (currentMode_ == DeviceMode::MODE_SERIAL) {
        if (serialPort_.isOpen()) serialPort_.close();
    }
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        CH347CloseDevice(ch347DeviceIndex_);
    }
    currentMode_ = DeviceMode::MODE_NONE;
}

bool PMBus::Configure(uint32_t bitrate) {
    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        int ret = SMBus_Configure(
            device_, static_cast<DWORD>(bitrate), DEFAULT_SLAVE_ADDR,
            DEFAULT_AUTO_READ_RESPOND, DEFAULT_WRITE_TIMEOUT_CP2112,
            DEFAULT_READ_TIMEOUT_CP2112, DEFAULT_SCL_LOW_TIMEOUT,
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
        if (executeSerialCommand(tx_data, rx_packet)) {
            return (rx_packet.payload[0] == 0x00);
        }
        lastError_ = "Serial configuration timeout.";
        return false;
    }
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        // 根据 Demo 匹配精确的波特率档位
        ULONG mode = 1; // 默认 100KHz
        if (bitrate >= 1000000) mode = 6;      // 1MHz
        else if (bitrate >= 750000) mode = 3;  // 750KHz
        else if (bitrate >= 400000) mode = 2;  // 400KHz
        else if (bitrate >= 200000) mode = 5;  // 200KHz
        else if (bitrate >= 100000) mode = 1;  // 100KHz
        else if (bitrate >= 50000) mode = 4;   // 50KHz
        else mode = 0;                         // 20KHz

        bool success = CH347I2C_Set(ch347DeviceIndex_, mode);
        // 根据 Demo 补充时钟拉伸配置
        success &= CH347I2C_SetStretch(ch347DeviceIndex_, TRUE);

        if (success) return true;

        lastError_ = "CH347T I2C Configure failed.";
        return false;
    }
    lastError_ = "Device not open";
    return false;
}

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

std::vector<int> PMBus::Flush() {
    std::vector<int> statusList(commandQueue_.size(), SLAVE_NOT_RESPONSE);
    if (!flushMode_ || currentMode_ != DeviceMode::MODE_SERIAL) return statusList;
    if (txBuffer_.empty()) return std::vector<int>();

    protocolParser_.reset();
    try { serialPort_.write(txBuffer_); }
    catch (const std::exception& e) {
        lastError_ = std::string("Serial flush write exception: ") + e.what();
        txBuffer_.clear();
        commandQueue_.clear();
        return statusList;
    }

    int receivedPackets = 0;
    int expectedPackets = commandQueue_.size();
    auto start_time = std::chrono::steady_clock::now();
    RPI2C::Packet rx_packet;

    while (receivedPackets < expectedPackets && std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(3000)) {
        try {
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
                                    if (cmd.useCrc) {
                                        std::vector<uint8_t> header = {
                                            static_cast<uint8_t>((cmd.slaveAddress << 1) & 0xFE), cmd.regAddr, static_cast<uint8_t>((cmd.slaveAddress << 1) | 1)
                                        };
                                        bool crcOk = VerifyPacketCrc(rx_packet.payload, header, cmd.crcType);
                                        int crcLen = GetCrcByteLength(cmd.crcType);
                                        if (rx_packet.payload.size() >= static_cast<size_t>(crcLen)) {
                                            rx_packet.payload.erase(rx_packet.payload.end() - crcLen, rx_packet.payload.end());
                                        }
                                        *(cmd.readResult) = rx_packet.payload;
                                        statusList[receivedPackets] = crcOk ? 0 : CRC_ERROR_CODE;
                                    }
                                    else {
                                        *(cmd.readResult) = rx_packet.payload;
                                        statusList[receivedPackets] = 0;
                                    }
                                }
                                else { statusList[receivedPackets] = SLAVE_NOT_RESPONSE; }
                            }
                            else {
                                if (rx_packet.payload.empty() || rx_packet.payload[0] != 0x00) {
                                    statusList[receivedPackets] = SLAVE_NOT_RESPONSE;
                                }
                                else { statusList[receivedPackets] = 0; }
                            }
                            receivedPackets++;
                        }
                    }
                }
            }
            else { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        }
        catch (...) { break; }
    }
    txBuffer_.clear();
    commandQueue_.clear();
    return statusList;
}

INT PMBus::Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    std::vector<uint8_t> actual_data = data;
    if (crcEnabled_) {
        std::vector<uint8_t> crc_payload = { static_cast<uint8_t>((slaveAddress << 1) & 0xFE), regAddr };
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
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        std::vector<uint8_t> buffer;
        buffer.push_back(slaveAddress << 1);
        buffer.push_back(regAddr);
        buffer.insert(buffer.end(), actual_data.begin(), actual_data.end());

        ULONG ackCount = 0;
        // 替换为 Demo 推荐的 RetACK 版本
        if (CH347StreamI2C_RetACK(ch347DeviceIndex_, buffer.size(), buffer.data(), 0, nullptr, &ackCount)) {
            if (ackCount > 0) return 0; // 只要有 ACK 就认为通讯链路没断
        }
        lastError_ = "CH347T Write NACK or failed.";
        return SLAVE_NOT_RESPONSE;
    }
    else {
        std::vector<uint8_t> payload = { regAddr };
        payload.insert(payload.end(), actual_data.begin(), actual_data.end());
        std::vector<uint8_t> tx_data = RPI2C::Protocol::packWrite(slaveAddress, payload);

        if (flushMode_) {
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
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
    int crcLen = GetCrcByteLength(crcType_);
    uint16_t actual_numBytes = crcEnabled_ ? numBytes + crcLen : numBytes;

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        std::vector<uint8_t> temp_result(actual_numBytes);
        int ret = SMBus_WriteRead(device_, temp_result.data(), slaveAddress << 1, static_cast<WORD>(actual_numBytes), static_cast<BYTE>(1), &regAddr);
        if (ret < 0) { lastError_ = "SMBus_WriteRead failed: " + std::to_string(ret); return ret; }

        if (crcEnabled_) {
            std::vector<uint8_t> header = { static_cast<uint8_t>((slaveAddress << 1) & 0xFE), regAddr, static_cast<uint8_t>((slaveAddress << 1) | 1) };
            bool crcOk = VerifyPacketCrc(temp_result, header, crcType_);
            if (temp_result.size() >= static_cast<size_t>(crcLen)) temp_result.erase(temp_result.end() - crcLen, temp_result.end());
            result = temp_result;
            if (!crcOk) { lastError_ = "SMBus Read CRC Error."; return CRC_ERROR_CODE; }
        }
        else { result = temp_result; }
        return ret;
    }
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        std::vector<uint8_t> writeBuf = { static_cast<uint8_t>(slaveAddress << 1), regAddr };
        std::vector<uint8_t> temp_result(actual_numBytes);
        ULONG ackCount = 0;

        // 替换为带 ACK 检验的函数
        if (CH347StreamI2C_RetACK(ch347DeviceIndex_, writeBuf.size(), writeBuf.data(), actual_numBytes, temp_result.data(), &ackCount)) {
            if (ackCount > 0) {
                if (crcEnabled_) {
                    std::vector<uint8_t> header = { static_cast<uint8_t>((slaveAddress << 1) & 0xFE), regAddr, static_cast<uint8_t>((slaveAddress << 1) | 1) };
                    bool crcOk = VerifyPacketCrc(temp_result, header, crcType_);
                    if (temp_result.size() >= static_cast<size_t>(crcLen)) temp_result.erase(temp_result.end() - crcLen, temp_result.end());
                    result = temp_result;
                    if (!crcOk) { lastError_ = "CH347T Read CRC Error."; return CRC_ERROR_CODE; }
                }
                else { result = temp_result; }
                return 0;
            }
        }
        lastError_ = "CH347T Read NACK or failed.";
        return SLAVE_NOT_RESPONSE;
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
                    std::vector<uint8_t> header = { static_cast<uint8_t>((slaveAddress << 1) & 0xFE), regAddr, static_cast<uint8_t>((slaveAddress << 1) | 1) };
                    bool crcOk = VerifyPacketCrc(rx_packet.payload, header, crcType_);
                    if (rx_packet.payload.size() >= static_cast<size_t>(crcLen)) rx_packet.payload.erase(rx_packet.payload.end() - crcLen, rx_packet.payload.end());
                    result = rx_packet.payload;
                    if (!crcOk) { lastError_ = "Serial I2C Read CRC Error."; return CRC_ERROR_CODE; }
                }
                else { result = rx_packet.payload; }
                return 0;
            }
            else { lastError_ = "Serial I2C Read NACK or Length mismatch."; return SLAVE_NOT_RESPONSE; }
        }
        return DEVICE_NOT_CONNECTED;
    }
}

INT PMBus::SendByte(uint8_t slaveAddress, uint8_t byte) {
    if (currentMode_ == DeviceMode::MODE_NONE) return DEVICE_NOT_CONNECTED;
    if (slaveAddress > 0x7F) { lastError_ = "Invalid slave address"; return DEVICE_NOT_CONNECTED; }

    std::vector<uint8_t> actual_data = { byte };
    if (crcEnabled_) {
        std::vector<uint8_t> crc_payload = { static_cast<uint8_t>((slaveAddress << 1) & 0xFE), byte };
        uint32_t calc_crc = I2CDebugger::CrcCalculator::Calculate(crcType_, crc_payload);
        AppendCrcToData(actual_data, calc_crc, crcType_);
    }

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        int ret = SMBus_Write(device_, actual_data.data(), slaveAddress << 1, static_cast<BYTE>(actual_data.size()));
        if (ret != 0) lastError_ = "SMBus_Write failed (SendByte): " + std::to_string(ret);
        return ret;
    }
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        std::vector<uint8_t> writeBuf;
        writeBuf.push_back(slaveAddress << 1);
        writeBuf.insert(writeBuf.end(), actual_data.begin(), actual_data.end());

        ULONG ackCount = 0;
        if (CH347StreamI2C_RetACK(ch347DeviceIndex_, writeBuf.size(), writeBuf.data(), 0, nullptr, &ackCount)) {
            if (ackCount > 0) return 0;
        }
        lastError_ = "CH347T SendByte NACK.";
        return SLAVE_NOT_RESPONSE;
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
    if (startAddr > 0x7F || endAddr > 0x7F) { lastError_ = "Invalid slave address"; return -1; }

    foundAddresses.clear();

    if (currentMode_ == DeviceMode::MODE_SMBUS) {
        std::vector<BYTE> addrGroup(128, 0);
        int ret = SMBus_Scan(device_, addrGroup.data(), startAddr, endAddr);
        if (ret < 0) { lastError_ = "SMBus_Scan failed: " + std::to_string(ret); return ret; }
        for (int i = 0; i <= endAddr - startAddr; ++i) {
            if (addrGroup[i] != 0) foundAddresses.push_back(static_cast<uint8_t>(addrGroup[i]));
        }
        return ret;
    }
    else if (currentMode_ == DeviceMode::MODE_CH347T) {
        // 参考 Demo 中的扫描逻辑
        for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
            uint8_t writeAddrByte = (addr << 1);
            ULONG ackCount = 0;
            if (CH347StreamI2C_RetACK(ch347DeviceIndex_, 1, &writeAddrByte, 0, nullptr, &ackCount)) {
                if (ackCount > 0) {
                    foundAddresses.push_back(addr);
                }
            }
        }
        return 0;
    }
    else {
        bool prevFlush = flushMode_;
        FlushOn();
        std::vector<std::vector<uint8_t>> scanResults(endAddr - startAddr + 1);

        for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
            std::vector<uint8_t> tx_data = RPI2C::Protocol::packRead(addr, 1);
            txBuffer_.insert(txBuffer_.end(), tx_data.begin(), tx_data.end());
            commandQueue_.push_back({ PendingTask::READ, &scanResults[addr - startAddr], 1 });
        }
        Flush();
        for (uint8_t addr = startAddr; addr <= endAddr; ++addr) {
            if (scanResults[addr - startAddr].size() == 1) foundAddresses.push_back(addr);
        }
        if (!prevFlush) FlushOff();
        return 0;
    }
}

std::string PMBus::GetLastError() const {
    return lastError_;
}

PMBus::DeviceMode PMBus::GetDeviceMode() const {
    return currentMode_;
}


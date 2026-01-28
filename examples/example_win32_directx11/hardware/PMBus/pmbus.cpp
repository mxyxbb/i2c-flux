// pmbus.cpp

#include "pmbus.h"
#include <cstring>
#include <stdexcept>

PMBus::PMBus() = default;

PMBus::~PMBus() {
    Close();
}

// 打开设备（传出设备名称，如 CP2112/123456）
bool PMBus::Open(char ** deviceName) {
    if (isOpen_) Close();

    int ret = SMBus_Open(&device_, deviceName);
    if (ret != 0) {
        lastError_ = "SMBus_Open failed with code: " + std::to_string(ret);
        return false;
    }
    isOpen_ = true;
    return true;
}

void PMBus::Close() {
    if (isOpen_) {
        SMBus_Close(device_);
        isOpen_ = false;
    }
}

bool PMBus::Configure(uint32_t bitrate) {
    if (!isOpen_) {
        lastError_ = "Device not open";
        return false;
    }

    int ret = SMBus_Configure(
        device_,
        static_cast<DWORD>(bitrate),         // bitRate
        DEFAULT_SLAVE_ADDR,                  // address
        DEFAULT_AUTO_READ_RESPOND,           // autoReadRespond
        DEFAULT_WRITE_TIMEOUT,               // writeTimeout
        DEFAULT_READ_TIMEOUT,                // readTimeout
        DEFAULT_SCL_LOW_TIMEOUT,             // sclLowTimeout
        DEFAULT_TRANSFER_RETRIES,            // transferRetries
        DEFAULT_RESPONSE_TIMEOUT             // responseTimeout
    );

    if (ret != 0) {
        lastError_ = "SMBus_Configure failed: " + std::to_string(ret);
        return false;
    }
    return true;
}

// slaveAddress: 7-bit address
INT PMBus::Write(uint8_t slaveAddress, uint8_t regAddr, const std::vector<uint8_t>& data) {
    if (!isOpen_) return false;
    if(slaveAddress > 0x7F) {
        lastError_ = "Invalid slave address";
        return false;
    }

    std::vector<uint8_t> buffer;
    buffer.push_back(regAddr);          // 寄存器地址
    buffer.insert(buffer.end(), data.begin(), data.end());

    int ret = SMBus_Write(
        device_,
        buffer.data(),
        slaveAddress<<1,
        static_cast<BYTE>(buffer.size())
    );

    if (ret != 0) {
        lastError_ = "SMBus_Write failed: " + std::to_string(ret);
        return ret;
    }
    return ret;
}

// slaveAddress: 7-bit address
INT PMBus::Read(uint8_t slaveAddress, uint8_t regAddr, uint16_t numBytes, std::vector<uint8_t>& result) {
    if (!isOpen_) return false;
    if (slaveAddress > 0x7F) {
        lastError_ = "Invalid slave address";
        return false;
    }
    result.resize(numBytes);

    int ret = SMBus_WriteRead(
        device_,
        result.data(),
        slaveAddress<<1,
        static_cast<WORD>(numBytes),
        static_cast<BYTE>(1),  // targetAddressSize = 1 byte (regAddr)
        &regAddr
    );

    if (ret < 0) {
        lastError_ = "SMBus_WriteRead failed: " + std::to_string(ret);
        return ret;
    }
    return ret;
}

// slaveAddress: 7-bit address
INT PMBus::SendByte(uint8_t slaveAddress, uint8_t byte) {
    if (!isOpen_) return false;
    if (slaveAddress > 0x7F) {
        lastError_ = "Invalid slave address";
        return false;
    }
    int ret = SMBus_Write(
        device_,
        &byte,
        slaveAddress<<1,
        static_cast<BYTE>(1)
    );
    if (ret != 0) {
        lastError_ = "SMBus_Write_Byte failed: " + std::to_string(ret);
        return ret;
    }
    return ret;
}

// slaveAddress: 7-bit address
INT PMBus::ScanDevices(uint8_t startAddr, uint8_t endAddr, std::vector<uint8_t>& foundAddresses) {
    if (!isOpen_) return -1;
    if (startAddr > 0x7F) {
        lastError_ = "Invalid slave address";
        return -1;
    }
    if (endAddr > 0x7F) {
        lastError_ = "Invalid slave address";
        return -1;
    }
    std::vector<BYTE> addrGroup(128, 0); // 假设最多 128 个地址
    int ret = SMBus_Scan(device_, addrGroup.data(), startAddr, endAddr);
    if (ret < 0) {
        lastError_ = "SMBus_Scan failed: " + std::to_string(ret);
        return ret;
    }

    foundAddresses.clear();
    for (int i = 0; i <= endAddr-startAddr; ++i) {
        if (addrGroup[i] != 0) { // 假设非零表示设备存在
            foundAddresses.push_back(static_cast<uint8_t>(addrGroup[i]));
        }
    }
    return ret;
}

std::string PMBus::GetLastError() const {
    return lastError_;
}

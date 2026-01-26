// hardware/i2c_simulator.h
#pragma once
#include <cstdint>
#include <vector>
#include <random>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>

class I2CSimulator {
public:
    I2CSimulator() {
        // 初始化一些模拟数据
        for (uint8_t addr = 0x20; addr <= 0x7F; addr++) {
            device_data[addr] = std::vector<uint8_t>(256, 0);
            // 填充一些随机数据
            for (int i = 0; i < 256; i++) {
                device_data[addr][i] = static_cast<uint8_t>(rand() % 256);
            }
        }
    }

    // 模拟I2C读取
    std::vector<uint8_t> read(uint8_t device_addr, uint8_t reg_addr, uint16_t len) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 模拟延迟

        std::lock_guard<std::mutex> lock(mutex_);

        if (device_data.find(device_addr) == device_data.end()) {
            return {}; // 设备不存在
        }

        std::vector<uint8_t> result;
        for (uint16_t i = 0; i < len; i++) {
            uint8_t addr = static_cast<uint8_t>(reg_addr + i);
            result.push_back(device_data[device_addr][addr]);
        }

        return result;
    }

    // 模拟I2C写入
    bool write(uint8_t device_addr, uint8_t reg_addr, const std::vector<uint8_t>& data) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        std::lock_guard<std::mutex> lock(mutex_);

        if (device_data.find(device_addr) == device_data.end()) {
            return false;
        }

        for (size_t i = 0; i < data.size(); i++) {
            uint8_t addr = static_cast<uint8_t>(reg_addr + i);
            device_data[device_addr][addr] = data[i];
        }

        return true;
    }

private:
    std::map<uint8_t, std::vector<uint8_t>> device_data;
    std::mutex mutex_;
};

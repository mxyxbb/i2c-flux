#pragma once
#include <vector>
#include <cstdint>

namespace I2CDebugger {
    class CrcCalculator {
    public:
        // 纯算法函数，没有任何外部依赖
        static uint32_t Calculate(int type, const std::vector<uint8_t>& data);
    };
}

#include "crc_calculator.h"

namespace I2CDebugger {
    uint32_t CrcCalculator::Calculate(int type, const std::vector<uint8_t>& data) {
        if (data.empty()) return 0;

        switch (type) {
        case 0: { // CRC-8 (SMBus)
            uint8_t crc = 0x00;
            for (uint8_t b : data) {
                crc ^= b;
                for (int i = 0; i < 8; i++) {
                    if (crc & 0x80) crc = (crc << 1) ^ 0x07;
                    else crc <<= 1;
                }
            }
            return crc;
        }
        case 1: { // CRC-16 (Modbus)
            uint16_t crc = 0xFFFF;
            for (uint8_t b : data) {
                crc ^= b;
                for (int i = 0; i < 8; i++) {
                    if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
                    else crc >>= 1;
                }
            }
            return crc;
        }
        case 2: { // CRC-32
            uint32_t crc = 0xFFFFFFFF;
            for (uint8_t b : data) {
                crc ^= b;
                for (int i = 0; i < 8; i++) {
                    if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
                    else crc >>= 1;
                }
            }
            return ~crc;
        }
        default: return 0;
        }
    }
}

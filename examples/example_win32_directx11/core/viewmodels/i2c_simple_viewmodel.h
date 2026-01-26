#pragma once

#include "../models/i2c_simple_app.h"
#include "../services/hardware_service.h"
#include <memory>

namespace I2CDebugger {

    class I2CSimpleViewModel {
    public:
        explicit I2CSimpleViewModel(std::shared_ptr<HardwareService> hardwareService);

        void Connect();
        void Disconnect();
        void ScanSlaves();
        void ExecuteOperation();
        void SelectSlave(int index);

        I2CSimpleAppData& GetData() { return m_data; }
        const I2CSimpleAppData& GetData() const { return m_data; }

        uint8_t ParseHexInput(const char* input) const;
        std::vector<uint8_t> ParseHexDataInput(const char* input) const;
        std::string FormatHexData(const std::vector<uint8_t>& data) const;

    private:
        I2CSimpleAppData m_data;
        std::shared_ptr<HardwareService> m_hardwareService;
    };

}

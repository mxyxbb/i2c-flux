#pragma once

#include <string>
#include <memory>

namespace I2CDebugger {

    struct I2CSimpleAppData;
    struct I2CTableAppData;

    class ConfigurationService {
    public:
        static std::shared_ptr<ConfigurationService> GetInstance();

        bool SaveGlobalConfiguration(const I2CSimpleAppData& simpleData,
            const I2CTableAppData& tableData,
            const std::string& filePath);

        bool LoadGlobalConfiguration(I2CSimpleAppData& simpleData,
            I2CTableAppData& tableData,
            const std::string& filePath);

        bool SaveCommandGroup(const I2CTableAppData& tableData,
            int groupIndex,
            const std::string& filePath);

        bool LoadCommandGroup(I2CTableAppData& tableData,
            const std::string& filePath,
            bool appendAsNew);

    private:
        ConfigurationService() = default;
        static std::shared_ptr<ConfigurationService> s_instance;
    };
}

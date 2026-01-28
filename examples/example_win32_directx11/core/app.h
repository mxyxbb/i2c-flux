#pragma once

#include <memory>
#include <string>

namespace I2CDebugger {

    class I2CSimpleViewModel;
    class I2CTableViewModel;
    class MainWindow;
    class HardwareService;
    class ConfigurationService;

    class App {
    public:
        App();
        ~App();

        void Initialize();
        void Render();
        void Shutdown();

        // 全局配置操作
        void SaveGlobalConfig();
        void LoadGlobalConfig();
        void LoadGlobalConfigFromFile(const std::string& filePath);

        // 获取可执行文件所在目录
        std::string GetExecutableDirectory() const;

    private:
        void RenderMainMenuBar();
        void AutoLoadConfig();

        std::shared_ptr<HardwareService> m_hardwareService;
        std::shared_ptr<I2CSimpleViewModel> m_simpleViewModel;
        std::shared_ptr<I2CTableViewModel> m_tableViewModel;
        std::unique_ptr<MainWindow> m_mainWindow;
        std::shared_ptr<ConfigurationService> m_configService;

        std::string m_globalConfigPath;
        bool m_showLoadGlobalPopup = false;
        char m_loadGlobalPathBuffer[512] = "";
    };

}

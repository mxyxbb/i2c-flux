#pragma once

#include <memory>

namespace I2CDebugger {

    class I2CSimpleViewModel;
    class I2CTableViewModel;
    class I2CSimpleWindow;
    class I2CTableWindow;

    class MainWindow {
    public:
        MainWindow(std::shared_ptr<I2CSimpleViewModel> simpleViewModel,
            std::shared_ptr<I2CTableViewModel> tableViewModel);
        ~MainWindow();

        // 初始化所有窗口的输入缓冲区
        void InitializeInputBuffers();

        void Render();

    private:
        std::unique_ptr<I2CSimpleWindow> m_simpleWindow;
        std::unique_ptr<I2CTableWindow> m_tableWindow;

        bool m_showSimpleWindow = true;
        bool m_showTableWindow = true;

    };

}

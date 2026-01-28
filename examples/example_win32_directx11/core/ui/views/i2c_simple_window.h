#pragma once

#include <memory>

namespace I2CDebugger {

    class I2CSimpleViewModel;

    class I2CSimpleWindow {
    public:
        explicit I2CSimpleWindow(std::shared_ptr<I2CSimpleViewModel> viewModel);
        void Render(bool* p_open = nullptr);  // 添加参数以支持窗口关闭控制

    private:
        void RenderDeviceConnection();
        void RenderSlaveScanner();
        void RenderSimpleOperation();

        std::shared_ptr<I2CSimpleViewModel> m_viewModel;
    };

}

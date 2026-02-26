#pragma once
#include <memory>
#include "core/viewmodels/plot_viewmodel.h"

namespace I2CDebugger {

    class PlotView {
    public:
        explicit PlotView(std::shared_ptr<PlotViewModel> viewModel);
        void Render();
        void RenderMenuBar();

    private:
        void RenderSettingsPanel(bool* p_open);
        void RenderPlotWindow(bool* p_open);

        std::shared_ptr<PlotViewModel> m_viewModel;
    };

} // namespace I2CDebugger

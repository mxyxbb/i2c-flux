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
        void ExportToCSV(const std::string& filepath);
        void ImportFromCSV(const std::string& filepath);
        
        std::shared_ptr<PlotViewModel> m_viewModel;

        float m_history_window = 10.0f;
        bool m_auto_scale_x = true;
        bool m_auto_scale_y = true;
        float m_paused_time = 0.0f;
        float m_last_view_min_x = 0.0f;
        float m_last_view_max_x = 10.0f;
        bool m_needs_fit_data = false;
    };

} // namespace I2CDebugger

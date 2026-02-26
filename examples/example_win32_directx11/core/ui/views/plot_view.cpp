#include "plot_view.h"
#include "imgui.h"
#include "core/implot/implot.h"

namespace I2CDebugger {

    // 用于 ImPlot 的自定义 Getter 数据结构
    struct CustomGetterData {
        const ScrollingBuffer* buffer;
        float scale;
    };

    // ImPlot 自定义 Getter 回调函数（应用动态缩放）
    static ImPlotPoint ChannelDataGetter(int idx, void* user_data) {
        auto* data = static_cast<CustomGetterData*>(user_data);
        const int actual_idx = (data->buffer->Offset + idx) % data->buffer->Data.size();
        const ImVec2& p = data->buffer->Data[actual_idx];
        return ImPlotPoint(p.x, p.y * data->scale); // 在渲染时应用缩放
    }

    PlotView::PlotView(std::shared_ptr<PlotViewModel> viewModel)
        : m_viewModel(viewModel) {
    }

    void PlotView::Render() {
        RenderMenuBar();
        if(m_viewModel->GetConfig().showPlotConfigWindow) {
            RenderSettingsPanel(&m_viewModel->GetConfig().showPlotConfigWindow);
        }
        if(m_viewModel->GetConfig().showPlotWindow) {
            RenderPlotWindow(&m_viewModel->GetConfig().showPlotWindow);
        }
    }

    void PlotView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("绘图")) {
                if (ImGui::MenuItem("绘图窗口", nullptr, &m_viewModel->GetConfig().showPlotWindow)) {
                }
                if (ImGui::MenuItem("绘图缩放\\颜色", nullptr, &m_viewModel->GetConfig().showPlotConfigWindow)) {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void PlotView::RenderSettingsPanel(bool* p_open) {
        if(!ImGui::Begin("Channel Settings", p_open)){
            ImGui::End();
            return;
        }

        auto& channels = m_viewModel->GetChannels();
        for (size_t i = 0; i < channels.size(); ++i) {
            auto& ch = channels[i];
            ImGui::PushID(static_cast<int>(i));

            // 可见性开关
            ImGui::Checkbox("##Visible", &ch->isVisible);
            ImGui::SameLine();

            // 颜色选择器
            ImGui::ColorEdit4("##Color", (float*)&ch->color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();

            // 通道名称与缩放
            ImGui::Text("%s", ch->name.c_str());
            ImGui::SameLine(150);
            ImGui::SetNextItemWidth(100);
            ImGui::SliderFloat("Scale", &ch->scale, 0.1f, 10.0f, "%.2f");

            ImGui::PopID();
        }

        ImGui::End();
    }

    void PlotView::RenderPlotWindow(bool* p_open) {
        if (!ImGui::Begin("Data Plot", p_open)) {
            ImGui::End();
            return;
        }

        // ================= 顶部控制栏 =================
        static float history_window = 10.0f; // 默认显示过去 10 秒
        static bool auto_scale_y = true;     // Y轴自动缩放


        // 2. 绘制微调减号按钮
        if (ImGui::Button("-##dec_time")) {
            history_window -= 0.1f;
            if (history_window < 1.0f) {
                history_window = 1.0f; // 边界保护
            }
        }

        // 3. 绘制微调加号按钮
        // 可以通过调整 SameLine 的间距让按钮紧凑一点
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::Button("+##inc_time")) {
            history_window += 0.1f;
            if (history_window > 60.0f) {
                history_window = 60.0f; // 边界保护
            }
        }

        // 1. 绘制滑块
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("时间窗口(s)", &history_window, 1.0f, 60.0f, "%.1f");

        // ================= 响应式布局换行逻辑 =================
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGuiStyle& style = ImGui::GetStyle();

        // 计算下一个控件（Checkbox）需要多少宽度
        float checkbox_width = ImGui::CalcTextSize("Y轴自动缩放").x + 40.0f;
        float last_item_x2 = ImGui::GetItemRectMax().x;

        // 判断剩余空间是否够放 Checkbox，够放就保持在同一行
        if (last_item_x2 + style.ItemSpacing.x + checkbox_width < window_visible_x2) {
            ImGui::SameLine();
        }
        ImGui::Checkbox("Y轴自动缩放", &auto_scale_y);

        // 判断剩余空间是否够放“暂停”按钮
        // 3. 【修改】暂停按钮逻辑
        float button_width = ImGui::CalcTextSize("暂停滚动").x + 30.0f;
        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + button_width <
            ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x) {
            ImGui::SameLine();
        }

        bool is_system_running = m_viewModel->IsSystemRunning();

        // 如果系统没在运行，禁用这个按钮使其不可点击
        if (!is_system_running) {
            ImGui::BeginDisabled();
        }

        if (m_viewModel->IsUserPaused()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
            if (ImGui::Button("继续滚动")) m_viewModel->SetUserPaused(false);
            ImGui::PopStyleColor();
        }
        else {
            if (ImGui::Button("暂停滚动")) m_viewModel->SetUserPaused(true);
        }

        // 恢复禁用状态
        if (!is_system_running) {
            ImGui::EndDisabled();
        }

        ImGui::Spacing(); // 增加一点垂直间距
        // ==============================================
        //
        // ================= X 轴时间计算 =================
        static float paused_time = 0.0f;

        // 【修改】只在处于“滚动”状态时更新时间，否则时间停在最后一刻
        if (m_viewModel->IsScrolling()) {
            paused_time = m_viewModel->GetRelativeTime();
        }

        float current_time = paused_time;

        // ================= 绘图区域 =================
        // ImVec2(-1, -1) 让绘图区域填满当前窗口剩余的所有空间
        if (ImPlot::BeginPlot("##Oscilloscope", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("Time (s)", "Value");

            // 1. 设置 X 轴动态滚动
            // 使用 ImGuiCond_Always 保证画面强制跟随 current_time 移动
            ImPlot::SetupAxisLimits(ImAxis_X1, current_time - history_window, current_time, ImGuiCond_Always);

            auto& channels = m_viewModel->GetChannels();

            // 2. 优化版的 Y 轴自动缩放（只参考可视范围内的点）
            if (auto_scale_y) {
                float min_y = FLT_MAX;
                float max_y = -FLT_MAX;
                bool has_data_in_view = false;

                for (const auto& ch : channels) {
                    if (ch->isVisible && !ch->buffer.Data.empty()) {
                        for (int i = 0; i < ch->buffer.Data.size(); ++i) {
                            const auto& point = ch->buffer.Data[i];

                            // 【关键优化】：只计算处于当前 X 轴视口范围内的数据！
                            if (point.x >= (current_time - history_window) && point.x <= current_time) {
                                has_data_in_view = true;
                                float scaled_y = point.y * ch->scale;
                                if (scaled_y < min_y) min_y = scaled_y;
                                if (scaled_y > max_y) max_y = scaled_y;
                            }
                        }
                    }
                }

                if (has_data_in_view) {
                    float range = max_y - min_y;
                    if (range <= 0.0f) range = 1.0f; // 防除0或死锁

                    float margin = range * 0.15f; // 上下各留 15% 裕量
                    ImPlot::SetupAxisLimits(ImAxis_Y1, min_y - margin, max_y + margin, ImGuiCond_Always);
                }
            }

            // 3. 渲染所有启用的通道曲线
            for (auto& ch : channels) {
                if (ch->isVisible && !ch->buffer.Data.empty()) {
                    ImPlot::SetNextLineStyle(ch->color);

                    CustomGetterData getterData = { &ch->buffer, ch->scale };
                    ImPlot::PlotLineG(
                        ch->name.c_str(),
                        ChannelDataGetter,
                        &getterData,
                        ch->buffer.Data.size()
                    );
                }
            }
            ImPlot::EndPlot();
        }

        ImGui::End();
    }
} // namespace I2CDebugger

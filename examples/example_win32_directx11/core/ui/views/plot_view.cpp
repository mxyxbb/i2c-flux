#include "plot_view.h"
#include "imgui.h"
#include "core/implot/implot.h"
#include <cfloat> // 用于 FLT_MAX
#include "core/implot/implot_internal.h" // 【新增】引入内部 API，用于获取图例状态

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
        if (m_viewModel->GetConfig().showPlotConfigWindow) {
            RenderSettingsPanel(&m_viewModel->GetConfig().showPlotConfigWindow);
        }
        if (m_viewModel->GetConfig().showPlotWindow) {
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
        if (!ImGui::Begin("Channel Settings", p_open)) {
            ImGui::End();
            return;
        }

        auto& channels = m_viewModel->GetChannels();
        for (size_t i = 0; i < channels.size(); ++i) {
            auto& ch = channels[i];
            ImGui::PushID(static_cast<int>(i));

            // 1. 可见性开关
            ImGui::Checkbox("##Visible", &ch->isVisible);
            ImGui::SameLine();

            // 2. 颜色选择器
            ImGui::ColorEdit4("##Color", (float*)&ch->color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();

            // 3. 通道名称
            ImGui::Text("%s", ch->name.c_str());

            // --- 以下是全新的缩放控制组件 ---

            // 动态计算右对齐，防止窗口变窄时控件重叠或布局错乱
            float controls_width = 150.0f; // 缩放组件的总大概宽度
            float available_space = ImGui::GetWindowContentRegionMax().x - ImGui::GetCursorPosX();
            if (available_space > controls_width) {
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - controls_width);
            }
            else {
                ImGui::SameLine(); // 空间不够就紧贴着放
            }

            // 4. 减小倍数按钮 (x0.5)
            if (ImGui::Button("-##scale_down", ImVec2(22, 0))) {
                ch->scale *= 0.5f;
                if (ch->scale < 0.001f) ch->scale = 0.001f; // 设置下限
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("缩小 (x0.5)");

            ImGui::SameLine(0, 2.0f); // 缩小控件之间的间距

            // 5. 对数拖拽输入框
            ImGui::SetNextItemWidth(70);
            // v_speed 设置为当前 scale 的 1%（0.01f），配合对数模式实现平滑拖拽
            ImGui::DragFloat("##Scale", &ch->scale, ch->scale * 0.01f, 0.001f, 10000.0f, "%.3fx", ImGuiSliderFlags_Logarithmic);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("左右拖拽平滑缩放\n双击或Ctrl+单击手动输入");

            ImGui::SameLine(0, 2.0f);

            // 6. 增大倍数按钮 (x2.0)
            if (ImGui::Button("+##scale_up", ImVec2(22, 0))) {
                ch->scale *= 2.0f;
                if (ch->scale > 10000.0f) ch->scale = 10000.0f; // 设置上限
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("放大 (x2.0)");

            // 7. 快捷复位按钮 (可选，恢复到 1.0x)
            ImGui::SameLine(0, 5.0f);
            if (ImGui::Button("R##reset", ImVec2(22, 0))) {
                ch->scale = 1.0f;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("复位到 1.0x");

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
        static bool auto_scale_x = true;     // 【修改】默认使能，保持窗口跟随最新时间
        static bool auto_scale_y = true;     // Y轴自动缩放

        // 【修改】如果关闭了 X 轴自动缩放，禁用时间窗口调节控件（因为此时由用户自由缩放）
        if (!auto_scale_x) {
            ImGui::BeginDisabled();
        }

        // 2. 绘制微调减号按钮
        if (ImGui::Button("-##dec_time")) {
            history_window -= 0.1f;
            if (history_window < 1.0f) {
                history_window = 1.0f; // 边界保护
            }
        }

        // 3. 绘制微调加号按钮
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

        if (!auto_scale_x) {
            ImGui::EndDisabled();
        }

        // ================= 响应式布局换行逻辑 =================
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGuiStyle& style = ImGui::GetStyle();

        // X轴自动缩放 Checkbox 的响应式布局
        float checkbox_x_width = ImGui::CalcTextSize("X轴自动缩放").x + 40.0f;
        float last_item_x2 = ImGui::GetItemRectMax().x;
        if (last_item_x2 + style.ItemSpacing.x + checkbox_x_width < window_visible_x2) {
            ImGui::SameLine();
        }
        ImGui::Checkbox("X轴自动缩放", &auto_scale_x);

        // Y轴自动缩放 Checkbox 的响应式布局
        float checkbox_y_width = ImGui::CalcTextSize("Y轴自动缩放").x + 40.0f;
        last_item_x2 = ImGui::GetItemRectMax().x;
        if (last_item_x2 + style.ItemSpacing.x + checkbox_y_width < window_visible_x2) {
            ImGui::SameLine();
        }
        ImGui::Checkbox("Y轴自动缩放", &auto_scale_y);

        // 暂停按钮逻辑
        float button_width = ImGui::CalcTextSize("暂停滚动").x + 30.0f;
        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + button_width < window_visible_x2) {
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

// ================= X 轴时间计算 =================
        static float paused_time = 0.0f;

        // 只在处于“滚动”状态时更新时间，否则时间停在最后一刻
        if (m_viewModel->IsScrolling()) {
            paused_time = m_viewModel->GetRelativeTime();
        }

        float current_time = paused_time;
        auto& channels = m_viewModel->GetChannels();

        // 【新增】用于保存上一帧真实的 X 轴视口范围
        static float last_view_min_x = 0.0f;
        static float last_view_max_x = 10.0f;

        // ================= 绘图区域 =================
        if (ImPlot::BeginPlot("##Oscilloscope", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("Time (s)", "Value");

            float view_min_x;
            float view_max_x;

            if (auto_scale_x) {
                // 使能时：强制使用设定的历史时间窗口
                view_min_x = current_time - history_window;
                view_max_x = current_time;
                ImPlot::SetupAxisLimits(ImAxis_X1, view_min_x, view_max_x, ImGuiCond_Always);
            }
            else {
                // 【修改】失能时：使用上一帧获取的 X 轴范围，千万不能在这里调用 GetPlotLimits()
                view_min_x = last_view_min_x;
                view_max_x = last_view_max_x;
            }

            // 2. 优化版的 Y 轴自动缩放（参考上一帧的可视范围）
            if (auto_scale_y) {
                float min_y = FLT_MAX;
                float max_y = -FLT_MAX;
                bool has_data_in_view = false;

                for (const auto& ch : channels) {
                    if (ch->isVisible && !ch->buffer.Data.empty()) {
                        for (int i = 0; i < ch->buffer.Data.size(); ++i) {
                            const auto& point = ch->buffer.Data[i];

                            if (point.x >= view_min_x && point.x <= view_max_x) {
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
                    if (range <= 0.0f) range = 1.0f; // 防除0

                    float margin = range * 0.15f; // 上下各留 15% 裕量
                    // 此时还在 Setup 阶段，调用 SetupAxisLimits 是安全的
                    ImPlot::SetupAxisLimits(ImAxis_Y1, min_y - margin, max_y + margin, ImGuiCond_Always);
                }
            }

            // 【关键修改】所有的 Setup 都已经完成！
            // 现在我们可以安全地获取（并隐式锁定）当前的 Limits，把它存下来给下一帧使用
            ImPlotRect limits = ImPlot::GetPlotLimits();
            last_view_min_x = limits.X.Min;
            last_view_max_x = limits.X.Max;

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

                    // ==========================================
                    // 【修改】：通过 ImPlot 内部 API 检查原生图例的隐藏状态
                    // 仅当曲线在原生图例中未被隐藏（Show == true）时，才绘制文本
                    // ==========================================
                    ImPlotItem* plot_item = ImPlot::GetItem(ch->name.c_str());
                    if (plot_item != nullptr && plot_item->Show) {

                        // 获取环形缓冲区中的最后一个点（最新数据）
                        int last_idx = (ch->buffer.Offset + ch->buffer.Data.size() - 1) % ch->buffer.Data.size();
                        ImVec2 last_point = ch->buffer.Data[last_idx];

                        // 仅当该最新点在当前 X 轴可视范围内时才显示文本
                        if (last_point.x >= view_min_x && last_point.x <= view_max_x) {
                            float scaled_y = last_point.y * ch->scale;

                            // 将图表坐标转换为屏幕像素坐标
                            ImVec2 pixel_pos = ImPlot::PlotToPixels(last_point.x, scaled_y);

                            char val_str[32];
                            std::snprintf(val_str, sizeof(val_str), "%.2f", last_point.y);
                            ImVec2 text_size = ImGui::CalcTextSize(val_str);

                            // 初始位置：放置在点的正上方，水平居中
                            pixel_pos.y -= (text_size.y + 6.0f);
                            pixel_pos.x -= (text_size.x * 0.5f);

                            // 获取当前绘图区域的屏幕边界
                            ImVec2 plot_pos = ImPlot::GetPlotPos();
                            ImVec2 plot_size = ImPlot::GetPlotSize();
                            float plot_right_edge = plot_pos.x + plot_size.x;
                            float plot_top_edge = plot_pos.y;

                            // 边界碰撞修正逻辑
                            if (pixel_pos.x + text_size.x > plot_right_edge) {
                                pixel_pos.x = plot_right_edge - text_size.x - 4.0f;
                            }

                            if (pixel_pos.y < plot_top_edge) {
                                pixel_pos.y = ImPlot::PlotToPixels(last_point.x, scaled_y).y + 6.0f;
                            }

                            // 绘制文字
                            ImPlot::GetPlotDrawList()->AddText(pixel_pos, ImGui::GetColorU32(ch->color), val_str);
                        }
                    }
                }
            }
            ImPlot::EndPlot();
        }

        ImGui::End();
    }
} // namespace I2CDebugger

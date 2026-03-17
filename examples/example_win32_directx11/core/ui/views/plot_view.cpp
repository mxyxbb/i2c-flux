#include "plot_view.h"
#include "imgui.h"
#include "core/implot/implot.h"
#include <cfloat> // 用于 FLT_MAX
#include "core/implot/implot_internal.h" 

// 【新增】用于文件读写和字符串处理
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
// 【新增】Windows 原生文件对话框头文件
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
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

    // ==========================================
    // 【新增】导出到 CSV 功能
    // ==========================================
    // 【新增】CSV 字段转义辅助函数
    static std::string EscapeCsvField(const std::string& field) {
        bool needsQuotes = field.find(',') != std::string::npos ||
            field.find('"') != std::string::npos ||
            field.find('\n') != std::string::npos ||
            field.find('\r') != std::string::npos;
        if (!needsQuotes) return field;

        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    // ==========================================
    // 【新增】调用 Windows 原生打开文件对话框
    // ==========================================
    static std::string OpenCSVDialog() {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(OPENFILENAMEA);
        ofn.hwndOwner = NULL; // 如果有主窗口句柄可以传进来，NULL也行
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        // OFN_NOCHANGEDIR 非常重要！防止对话框改变程序的当前工作目录
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return std::string(ofn.lpstrFile);
        }
#endif
        return ""; // 用户取消或失败
    }

    // ==========================================
    // 【新增】调用 Windows 原生保存文件对话框
    // ==========================================
    static std::string SaveCSVDialog() {
#ifdef _WIN32
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        // 设置默认文件名
        strcpy_s(szFile, "export_data.csv");

        ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(OPENFILENAMEA);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = "csv"; // 默认后缀
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE) {
            return std::string(ofn.lpstrFile);
        }
#endif
        return ""; // 用户取消或失败
    }
    // ==========================================
        // 【优化】导出到 CSV 功能
        // ==========================================
    void PlotView::ExportToCSV(const std::string& filepath) {
        // 使用二进制模式打开，避免跨平台的换行符转换问题
        std::ofstream file(filepath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!file.is_open()) return;

        // 写入 UTF-8 BOM，确保 Excel 正确识别编码（如中文通道名）
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        file.write(reinterpret_cast<const char*>(bom), sizeof(bom));

        auto& channels = m_viewModel->GetChannels();
        if (channels.empty()) return;

        std::ostringstream oss;

        // 1. 写入表头，并进行 CSV 转义
        for (size_t i = 0; i < channels.size(); ++i) {
            oss << EscapeCsvField(channels[i]->name + "_Time") << ","
                << EscapeCsvField(channels[i]->name + "_Value");
            if (i < channels.size() - 1) oss << ",";
        }
        oss << "\r\n"; // 显式写入 Windows 标准换行
        file << oss.str();

        // 2. 找到所有通道中最长的数据量
        size_t max_points = 0;
        for (const auto& ch : channels) {
            if (ch->buffer.Data.size() > max_points) {
                max_points = ch->buffer.Data.size();
            }
        }

        // 3. 逐行写入数据
        for (size_t i = 0; i < max_points; ++i) {
            oss.str(""); // 清空流
            oss.clear();

            for (size_t c = 0; c < channels.size(); ++c) {
                auto& ch = channels[c];
                if (i < ch->buffer.Data.size()) {
                    int actual_idx = (ch->buffer.Offset + i) % ch->buffer.Data.size();
                    ImVec2 pt = ch->buffer.Data[actual_idx];

                    // 使用固定的 6 位小数精度，防止出现科学计数法导致的数据不直观
                    oss << std::fixed << std::setprecision(6) << pt.x << "," << pt.y;
                }
                else {
                    oss << ","; // 数据较短的通道留空
                }

                if (c < channels.size() - 1) oss << ",";
            }
            oss << "\r\n";
            file << oss.str();
        }

        file.close();
    }

    // ==========================================
        // 【优化】从 CSV 导入功能（动态创建通道）
        // ==========================================
    void PlotView::ImportFromCSV(const std::string& filepath) {
        // 使用二进制模式读取
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open()) return;

        // 1. 尝试读取并跳过 UTF-8 BOM
        char bom[3] = { 0 };
        file.read(bom, 3);
        if (!(static_cast<unsigned char>(bom[0]) == 0xEF &&
            static_cast<unsigned char>(bom[1]) == 0xBB &&
            static_cast<unsigned char>(bom[2]) == 0xBF)) {
            file.seekg(0);
        }

        std::string line;
        // 2. 读取表头
        if (!std::getline(file, line)) return;

        // 剔除表头末尾的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // ================= 解析表头，重建通道 =================
        std::stringstream header_ss(line);
        std::string header_cell;
        std::vector<std::string> headers;

        // 按照逗号分割表头
        while (std::getline(header_ss, header_cell, ',')) {
            // 简单处理 CSV 转义带来的双引号 (例如 "CH1_Time" -> CH1_Time)
            if (header_cell.length() >= 2 && header_cell.front() == '"' && header_cell.back() == '"') {
                header_cell = header_cell.substr(1, header_cell.length() - 2);
            }
            headers.push_back(header_cell);
        }

        // 清空现有通道并重置时间
        m_viewModel->ClearChannels();
        m_viewModel->ResetTime();

        // 预设颜色表（与你的 StartPeriodicExecution 保持完全一致）
        std::vector<ImVec4> presetColors = {
            ImVec4(1.0f, 0.2f, 0.2f, 1.0f), // 红
            ImVec4(0.2f, 0.8f, 0.2f, 1.0f), // 绿
            ImVec4(0.2f, 0.5f, 1.0f, 1.0f), // 蓝
            ImVec4(1.0f, 0.8f, 0.0f, 1.0f), // 黄
            ImVec4(0.8f, 0.2f, 1.0f, 1.0f)  // 紫
        };
        int colorIndex = 0;

        // 我们导出的规则是 Time 和 Value 交替，因此通道数是列数的一半
        int channelCount = headers.size() / 2;

        for (int i = 0; i < channelCount; ++i) {
            // 取时间列的表头作为通道名称的基准
            std::string chName = headers[i * 2];

            // 去除我们导出时追加的 "_Time" 后缀，还原真实的通道名称
            size_t suffix_pos = chName.rfind("_Time");
            if (suffix_pos != std::string::npos) {
                chName = chName.substr(0, suffix_pos);
            }

            ImVec4 color = presetColors[colorIndex % presetColors.size()];
            colorIndex++;

            // 动态添加通道
            m_viewModel->AddChannel(i, chName, color);
        }
        // =======================================================

// 3. 逐行读取数据
        float min_t = FLT_MAX;
        float max_t = -FLT_MAX;

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string cell;
            std::vector<std::string> row;

            while (std::getline(ss, cell, ',')) {
                row.push_back(cell);
            }

            for (int c = 0; c < channelCount; ++c) {
                size_t time_col = c * 2;
                size_t val_col = c * 2 + 1;

                if (val_col < row.size() && !row[time_col].empty() && !row[val_col].empty()) {
                    try {
                        float t = std::stof(row[time_col]);
                        float v = std::stof(row[val_col]);

                        m_viewModel->AddDataPoint(c, t, v);

                        // 【新增】记录所有通道中时间的最小值和最大值
                        if (t < min_t) min_t = t;
                        if (t > max_t) max_t = t;

                    }
                    catch (const std::exception&) {
                        // 忽略解析失败的单元格
                    }
                }
            }
        }
        file.close();

        // ==========================================
        // 【新增】导入完成后同步更新绘图窗口的时间范围
        // ==========================================
        if (min_t <= max_t) {
            float range = max_t - min_t;
            if (range <= 0.0f) range = 1.0f; // 防除0

            // 1. 关闭 X 轴自动缩放，允许用户静态观察
            m_auto_scale_x = false;

            // 2. 将视口直接跳转到数据边界，左右加上 2% 的留白，视觉更舒服
            m_last_view_min_x = min_t - range * 0.02f;
            m_last_view_max_x = max_t + range * 0.02f;

            // 3. 更新系统历史窗口和暂停时间，保证逻辑同步
            m_history_window = range;
            if (m_history_window < 1.0f) m_history_window = 1.0f;
            m_paused_time = max_t;

            m_needs_fit_data = true;
        }

        // 导入完成后暂停滚动
        m_viewModel->SetUserPaused(true);
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
            // 【新增】文件菜单
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("导入 CSV...")) {
                    ImGui::OpenPopup("导入CSV");
                }
                if (ImGui::MenuItem("导出 CSV...")) {
                    ImGui::OpenPopup("导出CSV");
                }
                ImGui::EndMenu();
            }

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
            ImGui::DragFloat("##Scale", &ch->scale, ch->scale * 0.1f, 0.001f, 10000.0f, "%.3fx", ImGuiSliderFlags_Logarithmic);
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
        // 如果关闭了 X 轴自动缩放，禁用时间窗口调节控件（因为此时由用户自由缩放）
        if (!m_auto_scale_x) {
            ImGui::BeginDisabled();
        }

        // 2. 绘制微调减号按钮
        if (ImGui::Button("-##dec_time")) {
            m_history_window -= 0.1f;
            if (m_history_window < 1.0f) {
                m_history_window = 1.0f; // 边界保护
            }
        }

        // 3. 绘制微调加号按钮
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::Button("+##inc_time")) {
            m_history_window += 0.1f;
            if (m_history_window > 60.0f) {
                m_history_window = 60.0f; // 边界保护
            }
        }

        // 1. 绘制滑块
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("时间窗口(s)", &m_history_window, 1.0f, 60.0f, "%.1f");

        if (!m_auto_scale_x) {
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
        ImGui::Checkbox("X轴自动缩放", &m_auto_scale_x);

        // Y轴自动缩放 Checkbox 的响应式布局
        float checkbox_y_width = ImGui::CalcTextSize("Y轴自动缩放").x + 40.0f;
        last_item_x2 = ImGui::GetItemRectMax().x;
        if (last_item_x2 + style.ItemSpacing.x + checkbox_y_width < window_visible_x2) {
            ImGui::SameLine();
        }
        ImGui::Checkbox("Y轴自动缩放", &m_auto_scale_y);

        // 暂停按钮逻辑的响应式换行
        float button_width = ImGui::CalcTextSize("暂停滚动").x + 30.0f;
        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + button_width < window_visible_x2) {
            ImGui::SameLine();
        }

        bool is_system_running = m_viewModel->IsSystemRunning();



        // ==========================================
        // 【修改】根据暂停状态动态显示导入/导出按钮
        // ==========================================
        if (m_viewModel->IsUserPaused()) {
            // 如果系统没在运行，禁用这个按钮使其不可点击
            if (!is_system_running) {
                ImGui::BeginDisabled();
            }
            // 处于暂停状态：显示黄色的“继续滚动”按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
            if (ImGui::Button("继续滚动")) {
                m_viewModel->SetUserPaused(false);
            }
            ImGui::PopStyleColor();
            if (!is_system_running) {
                ImGui::EndDisabled();
            }
            // 紧接着在右侧追加导入和导出按钮
            ImGui::SameLine();
            if (ImGui::Button("导入 CSV")) {
                std::string path = OpenCSVDialog();
                if (!path.empty()) {
                    ImportFromCSV(path);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("导出 CSV")) {
                std::string path = SaveCSVDialog();
                if (!path.empty()) {
                    ExportToCSV(path);
                }
            }
        }
        else {
            if (!is_system_running) {
                ImGui::BeginDisabled();
            }
            // 处于滚动状态：只显示常规的“暂停滚动”按钮
            if (ImGui::Button("暂停滚动")) {
                m_viewModel->SetUserPaused(true);
            }
            if (!is_system_running) {
                ImGui::EndDisabled();
            }
        }

        ImGui::Spacing(); // 增加一点垂直间距
        // ==============================================

        // ================= X 轴时间计算 =================

        // 只在处于“滚动”状态时更新时间，否则时间停在最后一刻
        if (m_viewModel->IsScrolling()) {
            m_paused_time = m_viewModel->GetRelativeTime();
        }

        float current_time = m_paused_time;
        auto& channels = m_viewModel->GetChannels();

        // ================= 绘图区域 =================
        if (ImPlot::BeginPlot("##Oscilloscope", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("Time (s)", "Value");

            float view_min_x;
            float view_max_x;

            if (m_auto_scale_x) {
                // 使能时：强制使用设定的历史时间窗口
                view_min_x = current_time - m_history_window;
                view_max_x = current_time;
                ImPlot::SetupAxisLimits(ImAxis_X1, view_min_x, view_max_x, ImGuiCond_Always);
            }
            else if (m_needs_fit_data) {
                // 【新增】单次强制跳转：导入数据后，强制 ImPlot 跳转到我们计算好的数据边界
                view_min_x = m_last_view_min_x;
                view_max_x = m_last_view_max_x;
                ImPlot::SetupAxisLimits(ImAxis_X1, view_min_x, view_max_x, ImGuiCond_Always);

                // 执行完毕后立刻清除标志，把控制权还给用户的鼠标操作（拖拽/滚轮）
                m_needs_fit_data = false;
            }
            else {
                // 失能且无需强制跳转时：使用上一帧获取的 X 轴范围供 Y 轴自动缩放计算使用
                view_min_x = m_last_view_min_x;
                view_max_x = m_last_view_max_x;
                // 注意这里不要调用 SetupAxisLimits，让 ImPlot 自己处理鼠标交互
            }

            // 2. 优化版的 Y 轴自动缩放（参考上一帧的可视范围）
            if (m_auto_scale_y) {
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
            m_last_view_min_x = limits.X.Min;
            m_last_view_max_x = limits.X.Max;

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
                    // 通过 ImPlot 内部 API 检查原生图例的隐藏状态
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

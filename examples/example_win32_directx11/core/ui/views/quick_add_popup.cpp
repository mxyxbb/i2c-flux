#include "quick_add_popup.h"
#include "../../viewmodels/i2c_table_viewmodel.h"
#include "imgui.h"
#include <cstring>
#include "../../viewmodels/quick_add_processor.h" // <--- 引入这个

namespace I2CDebugger {

    void QuickAddPopup::Open(int tabType) {
        m_show = true;
        m_tabType = tabType;
        std::memset(m_buffer, 0, sizeof(m_buffer));
    }

    void QuickAddPopup::Render(std::shared_ptr<I2CTableViewModel> viewModel) {
        if (!m_show) return;

        ImGui::OpenPopup("快捷批量添加");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 520), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("快捷批量添加", &m_show, ImGuiWindowFlags_None)) {

            ImGui::Text("请输入逗号分隔的数据 (支持多行粘贴):");
            ImGui::InputTextMultiline("##quickaddtext", m_buffer, sizeof(m_buffer),
                ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 180));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "输入格式提示：数值可为空，但请保留逗号占位");
            if (m_tabType == 1 || m_tabType == 2) {
                ImGui::TextWrapped("【读取】格式: read, 寄存器地址(16进制), 读取长度, 读取公式, 别名\n"
                    "【写入】格式: write, 寄存器地址(16进制), 写入公式, 10进制数值, 别名\n"
                    "示例: read, 0x01, 2, (b1<<8)|b0, 温度\n"
                    "示例: write, 0x02, value*10, 25.5, 设定阈值");
            }
            else {
                ImGui::TextWrapped("寄存器表格式: 寄存器地址(16进制), 读取长度, 读取公式, 别名, 描述\n"
                    "示例: 0x0A, 2, (b1<<8)|b0, 气压, 气压传感器数据\n"
                    "示例: 0x0B, 1, ,,仅占位无解析");
            }

            ImGui::Spacing();

            if (ImGui::Button("确定添加", ImVec2(100, 0))) {
                // ========= 【这里是 MVVM 的核心体现】 =========
                // View 不关心数据怎么解析，只负责将用户的输入打包传给 ViewModel
                if (viewModel) {
                    // 直接调用静态 Processor，把脏活累活甩出去！
                    QuickAddProcessor::Process(viewModel, m_tabType, std::string(m_buffer));
                }
                // ==============================================

                m_show = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(100, 0))) {
                m_show = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

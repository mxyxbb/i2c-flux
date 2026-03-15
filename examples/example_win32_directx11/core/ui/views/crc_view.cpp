#include "crc_view.h"
#include "imgui.h"

namespace I2CDebugger {

    void CrcView::Open(std::shared_ptr<CrcViewModel> viewModel) {
        m_showPopup = true;
        std::memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
        if (viewModel) {
            viewModel->Reset();
        }
    }

    void CrcView::Render(std::shared_ptr<CrcViewModel> viewModel) {
        if (!m_showPopup || !viewModel) return;

        ImGui::OpenPopup("CRC 设置与测试");

        if (ImGui::BeginPopupModal("CRC 设置与测试", &m_showPopup, ImGuiWindowFlags_None)) {

            // --- 双向绑定：CRC 类型 ---
            ImGui::Text("算法类型:");
            ImGui::SameLine();
            const char* crcTypes[] = { "CRC-8 (SMBus/0x07)", "CRC-16 (Modbus/0xA001)", "CRC-32 (0xEDB88320)" };
            int currentType = viewModel->GetCrcType();

            //ImGui::SetNextItemWidth(200);
            if (ImGui::Combo("##crctype", &currentType, crcTypes, IM_ARRAYSIZE(crcTypes))) {
                viewModel->SetCrcType(currentType);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- 基础测试区块 ---
            ImGui::Text("基础算法测试:");
            ImGui::Text("输入数据 (Hex, 空格分隔):");
            //ImGui::SetNextItemWidth(300);
            ImGui::InputText("##crcinput", m_inputBuffer, sizeof(m_inputBuffer));

            if (ImGui::Button("计算基础 CRC")) {
                viewModel->ExecuteCalculation(m_inputBuffer);
            }
            ImGui::SameLine();
            ImGui::Text("结果: %s", viewModel->GetTestResult().c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- 模拟读写区块 ---
            ImGui::Text("模拟读写, 计算和对比CRC:");

            // 行1：从机地址 & 读写Combo
            ImGui::SetNextItemWidth(150);
            ImGui::InputText("从机地址", m_simSlaveAddr, sizeof(m_simSlaveAddr));
            ImGui::SameLine();
            const char* rwItems[] = { "写 (0)", "读 (1)" };
            ImGui::SetNextItemWidth(150);
            ImGui::Combo("读/写", &m_simRwMode, rwItems, IM_ARRAYSIZE(rwItems));

            // 行2：寄存器地址 & 长度
            ImGui::SetNextItemWidth(150);
            ImGui::InputText("寄存器地址", m_simRegAddr, sizeof(m_simRegAddr));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputText("长度", m_simLen, sizeof(m_simLen), ImGuiInputTextFlags_CharsDecimal);

            // 行3：数据输入
            //ImGui::SetNextItemWidth(270);
            ImGui::InputText("数据 (Hex)", m_simData, sizeof(m_simData));

            ImGui::Spacing();

            // 行4-7：展示参与计算的原始值与结果 (由 ViewModel 提供格式化好的字符串)
            ImGui::Text("字节1 (从机Addr<<1 | R/W) : %s", viewModel->GetSimByte1Str().c_str());
            ImGui::Text("字节2 (寄存器Addr)        : %s", viewModel->GetSimByte2Str().c_str());
            ImGui::Text("字节3、4... (数据)        : %s", viewModel->GetSimDataStr().c_str());

            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "计算结果 (CRC)           : %s", viewModel->GetSimCrcResult().c_str());

            // 触发模拟计算
            if (ImGui::Button("计算模拟 CRC")) {
                viewModel->ExecuteSimCalculation(m_simSlaveAddr, m_simRwMode, m_simRegAddr, m_simLen, m_simData);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- 关闭弹窗 ---
            if (ImGui::Button("关闭", ImVec2(80, 0))) {
                m_showPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

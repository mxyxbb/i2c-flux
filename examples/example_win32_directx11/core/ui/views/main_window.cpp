#include "main_window.h"
#include "i2c_simple_window.h"
#include "i2c_table_window.h"
#include "imgui.h"

namespace I2CDebugger {

    MainWindow::MainWindow(std::shared_ptr<I2CSimpleViewModel> simpleVM,
        std::shared_ptr<I2CTableViewModel> tableVM)
    {
        m_simpleWindow = std::make_unique<I2CSimpleWindow>(simpleVM);
        m_tableWindow = std::make_unique<I2CTableWindow>(tableVM);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::InitializeInputBuffers() {
        if (m_tableWindow) {
            m_tableWindow->InitializeInputBuffers();
        }
        // 如果简单窗口也需要，可以添加
        // if (m_simpleWindow) {
        // m_simpleWindow->InitializeInputBuffers();
        // }
    }

    // 你可以将这个函数放在你的 UI 渲染循环中（例如 RenderUI 函数内）
    void MainWindow::ShowAboutWindow1() {
        ImGui::OpenPopup("关于"); // 这里的 ID 必须和 BeginPopupModal 的名称一致

        // 设置弹窗在屏幕正中央显示
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // 2. 绘制模态弹窗
        // ImGuiWindowFlags_AlwaysAutoResize 会让窗口根据内容自动调整大小
        if (ImGui::BeginPopupModal("关于", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

            // 软件名称和版本号 (稍微放大或加粗字体，如果没有加载多字体这里就是普通文本)
            ImGui::Text("软件名称: I2C Flux");
            ImGui::Text("版本号: v1.0.0 (Develop)");

            ImGui::Separator(); // 画一条分割线

            // 作者和版权信息
            ImGui::Text("作者: XiZ");
            ImGui::Text("联系方式: xueyang.ma@xiz-tech.com");

            // 使用 TextColored 设置特定颜色 (例如灰色显示编译日期)
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "编译日期: %s %s", __DATE__, __TIME__);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Copyright (c) 2026 XiZ. All rights reserved.");

            ImGui::Separator();

            // 底部居中的关闭按钮
            // 计算按钮居中偏移：(窗口宽度 - 按钮宽度) / 2
            float buttonWidth = 120.0f;
            float windowWidth = ImGui::GetWindowSize().x;
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

            if (ImGui::Button("确定", ImVec2(buttonWidth, 0))) {
                m_showAboutWindow = false;
                ImGui::CloseCurrentPopup(); // 关闭弹窗
            }

            // 按回车键也可以触发默认按钮关闭
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
    }

    void MainWindow::Render()
    {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("窗口")) {
                ImGui::MenuItem("简单命令操作窗口", nullptr, &m_showSimpleWindow);
                ImGui::MenuItem("多命令表操作窗口", nullptr, &m_showTableWindow);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助")) {
                ImGui::MenuItem("关于", nullptr, &m_showAboutWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (m_showSimpleWindow) {
            m_simpleWindow->Render(&m_showSimpleWindow);
        }

        if (m_showTableWindow) {
            m_tableWindow->Render(&m_showTableWindow);
        }

        if(m_showAboutWindow){
            ShowAboutWindow1();
        }
    }
}

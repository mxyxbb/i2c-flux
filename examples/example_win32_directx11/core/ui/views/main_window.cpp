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

    void MainWindow::Render()
    {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("窗口")) {
                ImGui::MenuItem("简单命令操作窗口", nullptr, &m_showSimpleWindow);
                ImGui::MenuItem("多命令表操作窗口", nullptr, &m_showTableWindow);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助")) {
                if (ImGui::MenuItem("关于")) {
                }
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
    }

}

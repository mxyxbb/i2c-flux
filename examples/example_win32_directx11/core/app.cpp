#include "app.h"
#include "viewmodels/i2c_simple_viewmodel.h"
#include "viewmodels/i2c_table_viewmodel.h"
#include "ui/views/main_window.h"
#include "services/hardware_service.h"
#include "services/configuration_service.h"
#include "imgui.h"

#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace I2CDebugger {

    App::App() = default;
    App::~App() = default;

    std::string App::GetExecutableDirectory() const {
#ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("\\/");
        return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
#else
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        std::string fullPath(path, (count > 0) ? count : 0);
        size_t pos = fullPath.find_last_of('/');
        return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
#endif
    }

    void App::Initialize() {
        // 创建硬件服务
        m_hardwareService = std::make_shared<HardwareService>();

        // 创建 ViewModel
        m_simpleViewModel = std::make_shared<I2CSimpleViewModel>(m_hardwareService);
        m_tableViewModel = std::make_shared<I2CTableViewModel>(m_hardwareService);

        //========== 设置硬件服务回调 ==========

        // 连接回调 - 同步两个ViewModel的状态
        m_hardwareService->SetConnectCallback(
            [this](bool success, const std::string& deviceName, const std::string& errorMsg) {
                m_simpleViewModel->GetData().isConnected = success;
                m_simpleViewModel->GetData().deviceName = deviceName;
                if (!success && !errorMsg.empty()) {
                    m_simpleViewModel->GetData().lastOperationSuccess = false;
                    m_simpleViewModel->GetData().lastErrorMessage = errorMsg;
                }
                else if (success) {
                    m_simpleViewModel->GetData().lastOperationSuccess = true;
                    m_simpleViewModel->GetData().lastErrorMessage.clear();
                }
                m_tableViewModel->GetData().isConnected = success;
                m_tableViewModel->GetData().deviceName = deviceName;
            });

        // 设备异常断开回调
        m_hardwareService->SetDisconnectCallback([this]() {
            m_simpleViewModel->GetData().isConnected = false;
            m_simpleViewModel->GetData().deviceName.clear();
            m_simpleViewModel->GetData().isScanning = false;
            m_simpleViewModel->GetData().isOperating = false;
            m_simpleViewModel->GetData().lastOperationSuccess = false;
            m_simpleViewModel->GetData().lastErrorMessage = "设备已断开";

            m_tableViewModel->GetData().isConnected = false;
            m_tableViewModel->GetData().deviceName.clear();
            m_tableViewModel->GetData().isPeriodicRunning = false;
            // 重置寄存器表读取状态
            m_tableViewModel->GetData().isReadingAllRegisters = false;
            m_tableViewModel->GetData().isExecuteAllSingleCommands = false;
            });

        // 扫描回调
        m_hardwareService->SetScanCallback(
            [this](bool success, const std::vector<uint8_t>& slaves, const std::string& errorMsg) {
                m_simpleViewModel->GetData().isScanning = false;
                m_simpleViewModel->GetData().scannedSlaves.clear();

                if (success) {
                    for (uint8_t addr : slaves) {
                        SlaveInfo info;
                        info.address = addr;
                        info.selected = false;
                        m_simpleViewModel->GetData().scannedSlaves.push_back(info);
                    }m_simpleViewModel->GetData().lastOperationSuccess = true;
                    m_simpleViewModel->GetData().lastErrorMessage.clear();
                }
                else {
                    m_simpleViewModel->GetData().lastOperationSuccess = false;
                    m_simpleViewModel->GetData().lastErrorMessage = errorMsg.empty() ? "扫描失败" : errorMsg;
                }});

                // 数据回调 - 分发到两个ViewModel
                m_hardwareService->SetDataCallback([this](const ResponsePacket& packet) {
                    if (packet.controlId == 0) {
                        // 简单窗口的回调
                        m_simpleViewModel->GetData().isOperating = false;
                        m_simpleViewModel->GetData().lastOperationSuccess = packet.success;

                        // 触发活动指示灯
                        m_simpleViewModel->GetData().activityIndicator.Trigger();

                        if (packet.success) {
                            m_simpleViewModel->GetData().readData = packet.rawData;
                            m_simpleViewModel->GetData().lastErrorMessage.clear();
                        }
                        else {
                            m_simpleViewModel->GetData().lastErrorMessage =
                                packet.errorMsg.empty() ? "操作失败" : packet.errorMsg;
                        }
                    }
                    else {
                        // 多命令表窗口的回调
                        m_tableViewModel->OnDataResult(packet);
                    }
                    });

                // ========== 启动硬件服务工作线程 ==========
                m_hardwareService->Start();

                // 创建主窗口
                m_mainWindow = std::make_unique<MainWindow>(m_simpleViewModel, m_tableViewModel);

                // 初始化配置服务
                // 修改后（正确）
                m_configService = std::make_shared<ConfigurationService>();

                // 设置全局配置文件路径
                m_globalConfigPath = GetExecutableDirectory() + "i2c_debugger_config.json";

                // 自动加载配置
                AutoLoadConfig();

                // 配置加载完成后，初始化窗口的输入缓冲区
                m_mainWindow->InitializeInputBuffers();

    }

    void App::AutoLoadConfig() {
        m_configService->LoadGlobalConfiguration(
            m_simpleViewModel->GetData(),
            m_tableViewModel->GetData(),
            m_globalConfigPath
        );
    }

    void App::SaveGlobalConfig() {
        m_configService->SaveGlobalConfiguration(
            m_simpleViewModel->GetData(),
            m_tableViewModel->GetData(),
            m_globalConfigPath
        );
    }

    void App::LoadGlobalConfig() {
        m_configService->LoadGlobalConfiguration(
            m_simpleViewModel->GetData(),
            m_tableViewModel->GetData(),
            m_globalConfigPath
        );
    }

    void App::LoadGlobalConfigFromFile(const std::string& filePath) {
        m_configService->LoadGlobalConfiguration(
            m_simpleViewModel->GetData(),
            m_tableViewModel->GetData(),
            filePath
        );
    }

    void App::RenderMainMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("保存全局配置", "Ctrl+S")) {
                    SaveGlobalConfig();
                }
                if (ImGui::MenuItem("加载全局配置...")) {
                    m_showLoadGlobalPopup = true;
                    std::strncpy(m_loadGlobalPathBuffer, "", sizeof(m_loadGlobalPathBuffer));
                }
                ImGui::Separator();
                if (ImGui::MenuItem("退出", "Alt+F4")) {
                    // 退出前自动保存
                    SaveGlobalConfig();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("帮助")) {
                if (ImGui::MenuItem("关于")) {
                    // 显示关于对话框
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // 处理Ctrl+S快捷键
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            SaveGlobalConfig();
        }

        // 加载全局配置弹窗
        if (m_showLoadGlobalPopup) {
            ImGui::OpenPopup("加载全局配置");
        }

        if (ImGui::BeginPopupModal("加载全局配置", &m_showLoadGlobalPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("请输入配置文件路径:");
            ImGui::InputText("##loadpath", m_loadGlobalPathBuffer, sizeof(m_loadGlobalPathBuffer));

            ImGui::Spacing();

            if (ImGui::Button("浏览...", ImVec2(80, 0))) {
#ifdef _WIN32
                OPENFILENAMEA ofn;
                char szFile[512] = "";
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileNameA(&ofn)) {
                    std::strncpy(m_loadGlobalPathBuffer, szFile, sizeof(m_loadGlobalPathBuffer) - 1);
                }
#endif
            }

            ImGui::SameLine();
            if (ImGui::Button("加载", ImVec2(80, 0))) {
                if (strlen(m_loadGlobalPathBuffer) > 0) {
                    LoadGlobalConfigFromFile(m_loadGlobalPathBuffer);
                }
                m_showLoadGlobalPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showLoadGlobalPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void App::Render() {
        // 处理硬件服务回调（在UI线程中执行）
        m_hardwareService->ProcessCallbacks();

        // 渲染主菜单栏
        RenderMainMenuBar();

        // 渲染主窗口
        if (m_mainWindow) {
            m_mainWindow->Render();
        }
    }

    void App::Shutdown() {
        // 退出时自动保存全局配置
        SaveGlobalConfig();

        // 停止硬件服务工作线程
        if (m_hardwareService) {
            m_hardwareService->Stop();
        }
    }

}

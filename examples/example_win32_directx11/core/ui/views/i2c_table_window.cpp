#include "i2c_table_window.h"
#include "../../viewmodels/i2c_table_viewmodel.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>


#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>
#endif

namespace I2CDebugger {

    I2CTableWindow::I2CTableWindow(std::shared_ptr<I2CTableViewModel> viewModel)
        : m_viewModel(viewModel)
    {
        auto& group = m_viewModel->GetCurrentGroup();
        std::snprintf(m_slaveAddrInput, sizeof(m_slaveAddrInput), "0x%02X", group.slaveAddress);
        std::snprintf(m_intervalInput, sizeof(m_intervalInput), "%u", group.interval);
    }

    // 绘制活动指示灯
    static void DrawActivityIndicator(ActivityIndicator& indicator) {
        bool isOn = indicator.Update();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float radius = 6.0f;
        ImVec2 center(pos.x + radius + 2, pos.y + 3 + ImGui::GetTextLineHeight() / 2);

        if (isOn) {
            drawList->AddCircleFilled(center, radius, IM_COL32(0, 255, 0, 255));
            drawList->AddCircle(center, radius, IM_COL32(0, 200, 0, 255), 0, 2.0f);
        }
        else {
            drawList->AddCircleFilled(center, radius, IM_COL32(80, 80, 80, 255));
            drawList->AddCircle(center, radius, IM_COL32(60, 60, 60, 255), 0, 1.0f);
        }
        ImGui::Dummy(ImVec2(radius * 2 + 4, ImGui::GetTextLineHeight()));
    }

    // 获取状态颜色
    static ImVec4 GetStatusColor(bool success, ErrorType errorType) {
        if (success) {
            return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        else if (errorType == ErrorType::SlaveNotResponse) {
            return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        else if (errorType == ErrorType::DeviceDisconnected) {
            return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
    }

    // 获取状态文本
    static const char* GetStatusText(bool success, ErrorType errorType) {
        if (success) return "OK";
        switch (errorType) {
        case ErrorType::SlaveNotResponse: return "NAK";
        case ErrorType::DeviceDisconnected: return "断开";
        default: return "错误";
        }
    }

    void I2CTableWindow::RenderGroupSelector()
    {
        auto& data = m_viewModel->GetData();

        ImGui::Text("命令表:");
        ImGui::SameLine();

        std::vector<const char*> groupNames;
        for (const auto& g : data.commandGroups) {
            groupNames.push_back(g.name.c_str());
        }

        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo("##GroupSelect", &data.currentGroupIndex,
            groupNames.data(), static_cast<int>(groupNames.size()))) {
            auto& group = m_viewModel->GetCurrentGroup();
            std::snprintf(m_slaveAddrInput, sizeof(m_slaveAddrInput), "0x%02X", group.slaveAddress);
            std::snprintf(m_intervalInput, sizeof(m_intervalInput), "%u", group.interval);
        }

        ImGui::SameLine();
        if (ImGui::Button("添加")) { m_viewModel->AddGroup(); }
        ImGui::SameLine();
        if (ImGui::Button("重命名")) {
            m_showRenamePopup = true;
            auto& group = m_viewModel->GetCurrentGroup();
            std::strncpy(m_renameBuffer, group.name.c_str(), sizeof(m_renameBuffer) - 1);
        }
        ImGui::SameLine();
        if (ImGui::Button("删除")) { m_viewModel->DeleteGroup(); }
        ImGui::SameLine();
        if (ImGui::Button("导出")) {// 修改为"导出"
            m_showExportPopup = true;
            // 设置默认文件名
            auto& group = m_viewModel->GetCurrentGroup();
            std::snprintf(m_exportPathBuffer, sizeof(m_exportPathBuffer), "%s.json", group.name.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("导入")) {  // 修改为"导入"
            m_showImportPopup = true;
            std::strncpy(m_importPathBuffer, "", sizeof(m_importPathBuffer));
        }
    }

    void I2CTableWindow::Render(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("多命令表操作窗口", p_open)) {
            ImGui::End();
            return;
        }

        RenderGroupSelector();
        RenderSlaveAddressInput();
        ImGui::Separator();

        if (ImGui::BeginTabBar("CommandTabs")) {
            if (ImGui::BeginTabItem("寄存器表")) {
                m_viewModel->GetData().currentTab = TabType::RegisterTable;
                RenderRegisterTableTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("单次触发")) {
                m_viewModel->GetData().currentTab = TabType::SingleTrigger;
                RenderSingleTriggerTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("周期触发")) {
                m_viewModel->GetData().currentTab = TabType::PeriodicTrigger;
                RenderPeriodicTriggerTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        RenderPropertyPopup();
        RenderButtonNamePopup();
        RenderRenamePopup();
        // 渲染解析配置弹窗
        RenderParseConfigPopup();
        RenderExportPopup();
        RenderImportPopup();

        ImGui::End();
    }

    void I2CTableWindow::RenderExportPopup()
    {
        if (m_showExportPopup) {
            ImGui::OpenPopup("导出命令表");
        }

        if (ImGui::BeginPopupModal("导出命令表", &m_showExportPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("请选择导出路径:");
            ImGui::InputText("##exportpath", m_exportPathBuffer, sizeof(m_exportPathBuffer));

            ImGui::Spacing();

            if (ImGui::Button("浏览...", ImVec2(80, 0))) {
#ifdef _WIN32
                OPENFILENAMEA ofn;
                char szFile[512];
                std::strncpy(szFile, m_exportPathBuffer, sizeof(szFile) - 1);
                szFile[sizeof(szFile) - 1] = '\0';
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrDefExt = "json";
                ofn.Flags = OFN_OVERWRITEPROMPT;

                if (GetSaveFileNameA(&ofn)) {
                    std::strncpy(m_exportPathBuffer, szFile, sizeof(m_exportPathBuffer) - 1);
                }
#endif
            }

            ImGui::SameLine();
            if (ImGui::Button("导出", ImVec2(80, 0))) {
                if (strlen(m_exportPathBuffer) > 0) {
                    if (m_viewModel->ExportGroup(m_exportPathBuffer)) {
                        // 导出成功
                    }
                }
                m_showExportPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showExportPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::RenderImportPopup()
    {
        if (m_showImportPopup) {
            ImGui::OpenPopup("导入命令表");
        }

        if (ImGui::BeginPopupModal("导入命令表", &m_showImportPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("请选择要导入的JSON文件:");
            ImGui::InputText("##importpath", m_importPathBuffer, sizeof(m_importPathBuffer));

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
                    std::strncpy(m_importPathBuffer, szFile, sizeof(m_importPathBuffer) - 1);
                }
#endif
            }

            ImGui::SameLine();
            if (ImGui::Button("导入", ImVec2(80, 0))) {
                if (strlen(m_importPathBuffer) > 0) {
                    if (m_viewModel->ImportGroup(m_importPathBuffer)) {
                        // 导入成功，更新UI
                        auto& group = m_viewModel->GetCurrentGroup();
                        std::snprintf(m_slaveAddrInput, sizeof(m_slaveAddrInput), "0x%02X", group.slaveAddress);
                        std::snprintf(m_intervalInput, sizeof(m_intervalInput), "%u", group.interval);
                    }
                }
                m_showImportPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showImportPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::RenderSlaveAddressInput()
    {
        auto& data = m_viewModel->GetData();
        auto& group = m_viewModel->GetCurrentGroup1();

        ImGui::Text("从机地址:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputText("##TableSlaveAddr", m_slaveAddrInput, sizeof(m_slaveAddrInput))) {
            group.slaveAddress = m_viewModel->ParseHexInput(m_slaveAddrInput);
        }

        if (data.currentTab == TabType::PeriodicTrigger) {
            ImGui::SameLine();
            ImGui::Text("间隔(ms):");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (ImGui::InputText("##Interval", m_intervalInput, sizeof(m_intervalInput))) {
                group.interval = static_cast<uint32_t>(std::stoul(m_intervalInput));
            }
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        DrawActivityIndicator(data.activityIndicator);

        ImGui::SameLine();
        ImGui::Text("状态:");
        ImGui::SameLine();
        if (data.isConnected) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "已连接");
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "未连接");
        }
    }

    void I2CTableWindow::RenderRegisterTableTab()
    {
        auto& data = m_viewModel->GetData();
        auto& entries = m_viewModel->GetCurrentGroup1().registerEntries;

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        float tableHeight = ImGui::GetContentRegionAvail().y - 40;

        if (ImGui::BeginTable("RegisterTable", 7, flags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupColumn("序号", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("Reg地址", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("长度", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("寄存器值", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("寄存器描述", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("属性", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableHeadersRow();

            for (int i = 0; i < static_cast<int>(entries.size()); i++) {
                auto& entry = entries[i];
                ImGui::TableNextRow();
                ImGui::PushID(i);

                bool isSelected = (data.selectedRowRegister == i);

                ImGui::TableSetColumnIndex(0);
                char label[32];
                std::snprintf(label, sizeof(label), "%d", i + 1);
                if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_None, ImVec2(0, 0))) {
                    data.selectedRowRegister = i;
                }

                ImGui::TableSetColumnIndex(1);
                char regBuf[8];
                std::snprintf(regBuf, sizeof(regBuf), "0x%02X", entry.regAddress);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##reg", regBuf, sizeof(regBuf))) {
                    entry.regAddress = m_viewModel->ParseHexInput(regBuf);
                }

                ImGui::TableSetColumnIndex(2);
                char lenBuf[8];
                std::snprintf(lenBuf, sizeof(lenBuf), "%d", entry.length);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##len", lenBuf, sizeof(lenBuf))) {
                    entry.length = static_cast<uint8_t>(std::stoi(lenBuf));
                }

                ImGui::TableSetColumnIndex(3);
                std::string dataStr = m_viewModel->FormatHexData(entry.data);
                ImGui::TextUnformatted(dataStr.empty() ? "-" : dataStr.c_str());

                ImGui::TableSetColumnIndex(4);
                if (entry.data.empty() && entry.lastErrorType == ErrorType::None) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                }
                else {
                    ImVec4 color = GetStatusColor(entry.lastSuccess, entry.lastErrorType);
                    const char* statusText = GetStatusText(entry.lastSuccess, entry.lastErrorType);
                    ImGui::TextColored(color, "%s", statusText);
                    if (!entry.lastSuccess && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", entry.lastError.c_str());
                    }
                }

                ImGui::TableSetColumnIndex(5);
                char descBuf[128];
                std::strncpy(descBuf, entry.description.c_str(), sizeof(descBuf) - 1);
                descBuf[sizeof(descBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##desc", descBuf, sizeof(descBuf))) {
                    entry.description = descBuf;
                }

                ImGui::TableSetColumnIndex(6);
                if (ImGui::SmallButton("属性")) {
                    m_showPropertyPopup = true;
                    m_propertyEditIndex = i;
                    m_propertyTabType = 0;
                    m_propertyOverride = entry.overrideSlaveAddr;
                    std::snprintf(m_propertySlaveAddr, sizeof(m_propertySlaveAddr),
                        "0x%02X", entry.slaveAddress);
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();

        // 按钮禁用条件：未连接 或 正在读取
        bool canRead = data.isConnected && !data.isReadingAllRegisters;

        if (!canRead) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("读取所有寄存器", ImVec2(130, 0))) {
            m_viewModel->ReadAllRegisters();
        }

        if (!canRead) {
            ImGui::EndDisabled();
        }

        // 可选：添加提示信息
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (!data.isConnected) {
                ImGui::SetTooltip("请先连接设备");
            }
            else if (data.isReadingAllRegisters) {
                ImGui::SetTooltip("正在读取中，请等待完成...");
            }
        }

        float rightStart = ImGui::GetWindowWidth() - 320;
        ImGui::SameLine(rightStart);

        if (ImGui::Button("添加##reg", ImVec2(50, 0))) { m_viewModel->AddRegisterEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("删除##reg", ImVec2(50, 0))) { m_viewModel->DeleteRegisterEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("复制##reg", ImVec2(50, 0))) { m_viewModel->CopyRegisterEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("上移##reg", ImVec2(50, 0))) { m_viewModel->MoveRegisterEntryUp(); }
        ImGui::SameLine();
        if (ImGui::Button("下移##reg", ImVec2(50, 0))) { m_viewModel->MoveRegisterEntryDown(); }
    }

    void I2CTableWindow::RenderSingleTriggerTab()
    {
        auto& data = m_viewModel->GetData();
        auto& entries = m_viewModel->GetCurrentGroup1().singleTriggerEntries;

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        float tableHeight = ImGui::GetContentRegionAvail().y - 40;

        if (ImGui::BeginTable("SingleTriggerTable", 9, flags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupColumn("##En", ImGuiTableColumnFlags_WidthFixed, 25);
            ImGui::TableSetupColumn("序号", ImGuiTableColumnFlags_WidthFixed, 35);
            ImGui::TableSetupColumn("Reg地址", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("长度", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("寄存器值", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("延时", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 100);

            // 自定义Header行，支持全选checkbox
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            // 全选checkbox列
            ImGui::TableSetColumnIndex(0);
            {
                bool allEnabled = m_viewModel->AreAllSingleEntriesEnabled();
                bool anyEnabled = m_viewModel->AreAnySingleEntriesEnabled();
                // 手动处理混合状态显示
                bool checkValue = allEnabled;
                if (ImGui::Checkbox("##SelectAllSingle", &checkValue)) {
                    // 点击时：如果当前不是全选，则全选；否则全不选
                    if (anyEnabled && !allEnabled) {
                        m_viewModel->SetAllSingleEntriesEnabled(true);
                    }
                    else {
                        m_viewModel->SetAllSingleEntriesEnabled(checkValue);
                    }
                }
                // 如果是混合状态，在checkbox上绘制一个横线表示
                if (anyEnabled && !allEnabled) {
                    ImVec2 pos = ImGui::GetItemRectMin();
                    ImVec2 size = ImGui::GetItemRectSize();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    float lineY = pos.y + size.y * 0.5f;
                    float padding = 4.0f;
                    drawList->AddLine(
                        ImVec2(pos.x + padding, lineY),
                        ImVec2(pos.x + size.x - padding, lineY),
                        IM_COL32(200, 200, 200, 255), 2.0f);
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("全选/全不选");
                }
            }

            // 其他列header
            ImGui::TableSetColumnIndex(1);
            ImGui::TableHeader("序号");
            ImGui::TableSetColumnIndex(2);
            ImGui::TableHeader("Reg地址");
            ImGui::TableSetColumnIndex(3);
            ImGui::TableHeader("长度");
            ImGui::TableSetColumnIndex(4);
            ImGui::TableHeader("寄存器值");
            ImGui::TableSetColumnIndex(5);
            ImGui::TableHeader("状态");
            ImGui::TableSetColumnIndex(6);
            ImGui::TableHeader("延时");
            ImGui::TableSetColumnIndex(7);
            ImGui::TableHeader("类型");
            ImGui::TableSetColumnIndex(8);
            ImGui::TableHeader("操作");

            for (int i = 0; i < static_cast<int>(entries.size()); i++) {
                auto& entry = entries[i];
                ImGui::TableNextRow();
                ImGui::PushID(i + 1000);

                bool isSelected = (data.selectedRowSingle == i);

                ImGui::TableSetColumnIndex(0);
                ImGui::Checkbox("##en", &entry.enabled);

                ImGui::TableSetColumnIndex(1);
                char label[32];
                std::snprintf(label, sizeof(label), "%d", i + 1);
                if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_None, ImVec2(0, 0))) {
                    data.selectedRowSingle = i;
                }

                ImGui::TableSetColumnIndex(2);
                char regBuf[8];
                std::snprintf(regBuf, sizeof(regBuf), "0x%02X", entry.regAddress);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##reg", regBuf, sizeof(regBuf))) {
                    entry.regAddress = m_viewModel->ParseHexInput(regBuf);
                }

                ImGui::TableSetColumnIndex(3);
                char lenBuf[8];
                std::snprintf(lenBuf, sizeof(lenBuf), "%d", entry.length);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##len", lenBuf, sizeof(lenBuf))) {
                    entry.length = static_cast<uint8_t>(std::stoi(lenBuf));
                }

                ImGui::TableSetColumnIndex(4);
                std::string dataStr = m_viewModel->FormatHexData(entry.data);
                char dataBuf[256];
                std::strncpy(dataBuf, dataStr.c_str(), sizeof(dataBuf) - 1);
                dataBuf[sizeof(dataBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##data", dataBuf, sizeof(dataBuf))) {
                    entry.data = m_viewModel->ParseHexDataInput(dataBuf);
                }

                ImGui::TableSetColumnIndex(5);
                if (entry.data.empty() && entry.lastErrorType == ErrorType::None) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                }
                else {
                    ImVec4 color = GetStatusColor(entry.lastSuccess, entry.lastErrorType);
                    const char* statusText = GetStatusText(entry.lastSuccess, entry.lastErrorType);
                    ImGui::TextColored(color, "%s", statusText);
                    if (!entry.lastSuccess && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", entry.lastError.c_str());
                    }
                }

                ImGui::TableSetColumnIndex(6);
                char delayBuf[16];
                std::snprintf(delayBuf, sizeof(delayBuf), "%u", entry.delayMs);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##delay", delayBuf, sizeof(delayBuf))) {
                    entry.delayMs = static_cast<uint32_t>(std::stoul(delayBuf));
                }

                ImGui::TableSetColumnIndex(7);
                const char* typeItems[] = { "读取", "写入", "命令" };
                int currentType = static_cast<int>(entry.type);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##type", &currentType, typeItems, 3)) {
                    entry.type = static_cast<CommandType>(currentType);
                }

                ImGui::TableSetColumnIndex(8);
                if (ImGui::SmallButton(entry.buttonName.c_str())) {
                    m_viewModel->ExecuteSingleCommand(i);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_showButtonNamePopup = true;
                    m_buttonNameEditIndex = i;
                    m_buttonNameTabType = 1;
                    std::strncpy(m_buttonNameBuffer, entry.buttonName.c_str(), sizeof(m_buttonNameBuffer) - 1);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("属性")) {
                    m_showPropertyPopup = true;
                    m_propertyEditIndex = i;
                    m_propertyTabType = 1;
                    m_propertyOverride = entry.overrideSlaveAddr;
                    std::snprintf(m_propertySlaveAddr, sizeof(m_propertySlaveAddr),
                        "0x%02X", entry.slaveAddress);
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();

        // 按钮禁用条件：未连接 或 正在读取
        bool canRead = data.isConnected && !data.isExecuteAllSingleCommands;

        if (!canRead) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("执行所有命令", ImVec2(120, 0))) {
            m_viewModel->ExecuteAllSingleCommands();
        }

        if (!canRead) {
            ImGui::EndDisabled();
        }

        // 添加提示信息
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (!data.isConnected) {
                ImGui::SetTooltip("请先连接设备");
            }
            else if (data.isExecuteAllSingleCommands) {
                ImGui::SetTooltip("正在读取中，请等待完成...");
            }
        }

        float rightStart = ImGui::GetWindowWidth() - 320;
        ImGui::SameLine(rightStart);

        if (ImGui::Button("添加##single", ImVec2(50, 0))) { m_viewModel->AddSingleEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("删除##single", ImVec2(50, 0))) { m_viewModel->DeleteSingleEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("复制##single", ImVec2(50, 0))) { m_viewModel->CopySingleEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("上移##single", ImVec2(50, 0))) { m_viewModel->MoveSingleEntryUp(); }
        ImGui::SameLine();
        if (ImGui::Button("下移##single", ImVec2(50, 0))) { m_viewModel->MoveSingleEntryDown(); }
    }

    void I2CTableWindow::RenderPeriodicTriggerTab()
    {
        auto& data = m_viewModel->GetData();
        auto& entries = m_viewModel->GetCurrentGroup1().periodicTriggerEntries;

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        float tableHeight = ImGui::GetContentRegionAvail().y - 40;

        // 增加解析值列，共12列
        if (ImGui::BeginTable("PeriodicTriggerTable", 12, flags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupColumn("##En", ImGuiTableColumnFlags_WidthFixed, 25);
            ImGui::TableSetupColumn("序号", ImGuiTableColumnFlags_WidthFixed, 35);
            ImGui::TableSetupColumn("Reg地址", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("长度", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("寄存器值(Raw)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("解析值", ImGuiTableColumnFlags_WidthFixed, 80);  // 新增解析值列
            ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("NAK", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableSetupColumn("延时", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 70);
            ImGui::TableSetupColumn("曲线", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 100);

            // 自定义Header行，支持全选checkbox
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

            // 全选checkbox列
            ImGui::TableSetColumnIndex(0);
            {
                bool allEnabled = m_viewModel->AreAllPeriodicEntriesEnabled();
                bool anyEnabled = m_viewModel->AreAnyPeriodicEntriesEnabled();

                bool checkValue = allEnabled;
                if (ImGui::Checkbox("##SelectAllPeriodic", &checkValue)) {
                    if (anyEnabled && !allEnabled) {
                        m_viewModel->SetAllPeriodicEntriesEnabled(true);
                    }
                    else {
                        m_viewModel->SetAllPeriodicEntriesEnabled(checkValue);
                    }
                }

                if (anyEnabled && !allEnabled) {
                    ImVec2 pos = ImGui::GetItemRectMin();
                    ImVec2 size = ImGui::GetItemRectSize();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    float lineY = pos.y + size.y * 0.5f;
                    float padding = 4.0f;
                    drawList->AddLine(
                        ImVec2(pos.x + padding, lineY),
                        ImVec2(pos.x + size.x - padding, lineY),
                        IM_COL32(200, 200, 200, 255), 2.0f);
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("全选/全不选");
                }
            }

            // 其他列header
            ImGui::TableSetColumnIndex(1);  ImGui::TableHeader("序号");
            ImGui::TableSetColumnIndex(2);  ImGui::TableHeader("Reg地址");
            ImGui::TableSetColumnIndex(3);  ImGui::TableHeader("长度");
            ImGui::TableSetColumnIndex(4);  ImGui::TableHeader("寄存器值(Raw)");
            ImGui::TableSetColumnIndex(5);  ImGui::TableHeader("解析值");
            ImGui::TableSetColumnIndex(6);  ImGui::TableHeader("状态");
            ImGui::TableSetColumnIndex(7);  ImGui::TableHeader("NAK");
            ImGui::TableSetColumnIndex(8);  ImGui::TableHeader("延时");
            ImGui::TableSetColumnIndex(9);  ImGui::TableHeader("类型");
            ImGui::TableSetColumnIndex(10); ImGui::TableHeader("曲线");
            ImGui::TableSetColumnIndex(11); ImGui::TableHeader("操作");

            for (int i = 0; i < static_cast<int>(entries.size()); i++) {
                auto& entry = entries[i];
                ImGui::TableNextRow();
                ImGui::PushID(i + 2000);

                bool isSelected = (data.selectedRowPeriodic == i);
                bool isReadType = (entry.type == CommandType::Read);
                bool isWriteType = (entry.type == CommandType::Write);

                // 列0: 启用
                ImGui::TableSetColumnIndex(0);
                ImGui::Checkbox("##en", &entry.enabled);

                // 列1: 序号
                ImGui::TableSetColumnIndex(1);
                char label[32];
                std::snprintf(label, sizeof(label), "%d", i + 1);
                if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_None, ImVec2(0, 0))) {
                    data.selectedRowPeriodic = i;
                }

                // 列2: Reg地址
                ImGui::TableSetColumnIndex(2);
                char regBuf[8];
                std::snprintf(regBuf, sizeof(regBuf), "0x%02X", entry.regAddress);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##reg", regBuf, sizeof(regBuf))) {
                    entry.regAddress = m_viewModel->ParseHexInput(regBuf);
                }

                // 列3: 长度
                ImGui::TableSetColumnIndex(3);
                char lenBuf[8];
                std::snprintf(lenBuf, sizeof(lenBuf), "%d", entry.length);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##len", lenBuf, sizeof(lenBuf))) {
                    entry.length = static_cast<uint8_t>(std::stoi(lenBuf));
                }

                // 列4: 寄存器值(Raw) - 可编辑
                ImGui::TableSetColumnIndex(4);
                std::string dataStr = m_viewModel->FormatHexData(entry.data);
                char dataBuf[256];
                std::strncpy(dataBuf, dataStr.c_str(), sizeof(dataBuf) - 1);
                dataBuf[sizeof(dataBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##data", dataBuf, sizeof(dataBuf))) {
                    entry.data = m_viewModel->ParseHexDataInput(dataBuf);
                    // 编辑Raw时，自动更新解析值（使用读取公式）
                    if (entry.parseConfigured && !entry.formula.empty()) {
                        m_viewModel->UpdateParsedValue(i);
                    }
                }

                // 列5: 解析值 - 根据命令类型决定是否可编辑
                ImGui::TableSetColumnIndex(5);
                if (entry.parseConfigured) {
                    if (isWriteType) {
                        // 写入命令：解析值可编辑，编辑后使用写入公式更新Raw
                        char parsedBuf[64];
                        std::snprintf(parsedBuf, sizeof(parsedBuf), "%.4g", entry.parsedValue);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputText("##parsed", parsedBuf, sizeof(parsedBuf),
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
                            try {
                                double newValue = std::stod(parsedBuf);
                                m_viewModel->UpdateRawFromParsedValue(i, newValue);
                            }
                            catch (...) {
                                // 输入无效，忽略
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("输入十进制值，按Enter确认\n将使用写入公式转换为Raw数据");
                        }
                    }
                    else if (isReadType) {
                        // 读取命令：只显示解析值
                        if (entry.parseSuccess) {
                            ImGui::Text("%.4g", entry.parsedValue);
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ERR");
                            if (ImGui::IsItemHovered() && !entry.lastError.empty()) {
                                ImGui::SetTooltip("解析错误: %s", entry.lastError.c_str());
                            }
                        }
                    }
                    else {
                        ImGui::TextDisabled("--");
                    }
                }
                else {
                    ImGui::TextDisabled("--");
                }

                // 列6: 状态
                ImGui::TableSetColumnIndex(6);
                if (entry.data.empty() && entry.lastErrorType == ErrorType::None) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                }
                else {
                    ImVec4 color = GetStatusColor(entry.lastSuccess, entry.lastErrorType);
                    const char* statusText = GetStatusText(entry.lastSuccess, entry.lastErrorType);
                    ImGui::TextColored(color, "%s", statusText);
                    if (!entry.lastSuccess && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", entry.lastError.c_str());
                    }
                }

                // 列7: 错误计数（NAK次数）
                ImGui::TableSetColumnIndex(7);
                if (entry.errorCount > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%u", entry.errorCount);
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "0");
                }

                // 列8: 延时
                ImGui::TableSetColumnIndex(8);
                char delayBuf[16];
                std::snprintf(delayBuf, sizeof(delayBuf), "%u", entry.delayMs);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##delay", delayBuf, sizeof(delayBuf))) {
                    entry.delayMs = static_cast<uint32_t>(std::stoul(delayBuf));
                }

                // 列9: 命令类型
                ImGui::TableSetColumnIndex(9);
                const char* typeItems[] = { "读取", "写入", "命令" };
                int currentType = static_cast<int>(entry.type);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##type", &currentType, typeItems, 3)) {
                    entry.type = static_cast<CommandType>(currentType);
                }

                // 列10: 曲线列
                ImGui::TableSetColumnIndex(10);
                if (isReadType) {
                    if (ImGui::SmallButton("解析")) {
                        m_showParsePopup = true;
                        m_parseEditIndex = i;
                        std::strncpy(m_aliasBuffer, entry.alias.c_str(), sizeof(m_aliasBuffer) - 1);
                        m_aliasBuffer[sizeof(m_aliasBuffer) - 1] = '\0';
                        std::strncpy(m_readFormulaInput, entry.formula.c_str(), sizeof(m_readFormulaInput) - 1);
                        m_readFormulaInput[sizeof(m_readFormulaInput) - 1] = '\0';
                        std::strncpy(m_writeFormulaInput, entry.writeFormula.c_str(), sizeof(m_writeFormulaInput) - 1);
                        m_writeFormulaInput[sizeof(m_writeFormulaInput) - 1] = '\0';
                    }
                    ImGui::SameLine();
                    if (entry.parseConfigured) {
                        ImGui::Checkbox("##curve", &entry.showCurve);
                    }
                    else {
                        ImGui::BeginDisabled();
                        bool temp = false;
                        ImGui::Checkbox("##curve", &temp);
                        ImGui::EndDisabled();
                    }
                }
                else if (isWriteType) {
                    // 写入命令也可以配置解析（用于十进制输入转Raw）
                    if (ImGui::SmallButton("解析")) {
                        m_showParsePopup = true;
                        m_parseEditIndex = i;
                        std::strncpy(m_aliasBuffer, entry.alias.c_str(), sizeof(m_aliasBuffer) - 1);
                        m_aliasBuffer[sizeof(m_aliasBuffer) - 1] = '\0';
                        std::strncpy(m_readFormulaInput, entry.formula.c_str(), sizeof(m_readFormulaInput) - 1);
                        m_readFormulaInput[sizeof(m_readFormulaInput) - 1] = '\0';
                        std::strncpy(m_writeFormulaInput, entry.writeFormula.c_str(), sizeof(m_writeFormulaInput) - 1);
                        m_writeFormulaInput[sizeof(m_writeFormulaInput) - 1] = '\0';
                    }
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    bool temp = false;
                    ImGui::Checkbox("##curve", &temp);
                    ImGui::EndDisabled();
                }
                else {
                    ImGui::BeginDisabled();
                    ImGui::SmallButton("解析");
                    ImGui::SameLine();
                    bool temp = false;
                    ImGui::Checkbox("##curve", &temp);
                    ImGui::EndDisabled();
                }

                // 列11: 操作按钮
                ImGui::TableSetColumnIndex(11);
                if (ImGui::SmallButton(entry.buttonName.c_str())) {
                    m_viewModel->ExecutePeriodicCommand(i);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_showButtonNamePopup = true;
                    m_buttonNameEditIndex = i;
                    m_buttonNameTabType = 2;
                    std::strncpy(m_buttonNameBuffer, entry.buttonName.c_str(), sizeof(m_buttonNameBuffer) - 1);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("属性")) {
                    m_showPropertyPopup = true;
                    m_propertyEditIndex = i;
                    m_propertyTabType = 2;
                    m_propertyOverride = entry.overrideSlaveAddr;
                    std::snprintf(m_propertySlaveAddr, sizeof(m_propertySlaveAddr),
                        "0x%02X", entry.slaveAddress);
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        // 底部按钮
        ImGui::Spacing();

        bool canRead = data.isConnected;

        if (!canRead) {
            ImGui::BeginDisabled();
        }

        if (data.isPeriodicRunning) {
            if (ImGui::Button("停止周期触发", ImVec2(120, 0))) {
                m_viewModel->StopPeriodicExecution();
            }
        }
        else {
            if (ImGui::Button("开始周期触发", ImVec2(120, 0))) {
                m_viewModel->StartPeriodicExecution();
            }
        }

        if (!canRead) {
            ImGui::EndDisabled();
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (!data.isConnected) {
                ImGui::SetTooltip("请先连接设备");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("清零NAK计数", ImVec2(120, 0))) {
            m_viewModel->ResetPeriodicErrorCounts();
        }

        float rightStart = ImGui::GetWindowWidth() - 320;
        ImGui::SameLine(rightStart);

        if (ImGui::Button("添加##periodic", ImVec2(50, 0))) { m_viewModel->AddPeriodicEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("删除##periodic", ImVec2(50, 0))) { m_viewModel->DeletePeriodicEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("复制##periodic", ImVec2(50, 0))) { m_viewModel->CopyPeriodicEntry(); }
        ImGui::SameLine();
        if (ImGui::Button("上移##periodic", ImVec2(50, 0))) { m_viewModel->MovePeriodicEntryUp(); }
        ImGui::SameLine();
        if (ImGui::Button("下移##periodic", ImVec2(50, 0))) { m_viewModel->MovePeriodicEntryDown(); }

    }

    void I2CTableWindow::RenderParseConfigPopup()
    {
        if (!m_showParsePopup) return;

        auto& entries = m_viewModel->GetCurrentGroup1().periodicTriggerEntries;
        if (m_parseEditIndex < 0 || m_parseEditIndex >= static_cast<int>(entries.size())) {
            m_showParsePopup = false;
            return;
        }

        ImGui::OpenPopup("解析配置");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(450, 380), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("解析配置", &m_showParsePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            auto& entry = entries[m_parseEditIndex];

            // 别名
            ImGui::Text("别名 (用于曲线图例和CSV表头):");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##Alias", m_aliasBuffer, sizeof(m_aliasBuffer));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 读取公式
            ImGui::Text("读取公式 (Raw字节 → 十进制值):");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##ReadFormula", m_readFormulaInput, sizeof(m_readFormulaInput));
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "示例: (b1 << 8) | b0, w0 * 0.1, b0 / 10.0");

            ImGui::Spacing();

            // 写入公式
            ImGui::Text("写入公式 (十进制值 → Raw字节):");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##WriteFormula", m_writeFormulaInput, sizeof(m_writeFormulaInput));
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "示例: value, value * 10, value / 0.1");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 公式帮助
            if (ImGui::CollapsingHeader("公式变量说明")) {
                ImGui::TextWrapped(
                    "=== 读取公式变量 ===\n"
                    "b0, b1, b2... : 第1, 2, 3...个字节\n"
                    "w0, w1...     : 小端字, w0 = (b1<<8)|b0\n"
                    "\n"
                    "=== 写入公式变量 ===\n"
                    "value 或 v    : 输入的十进制值\n"
                    "\n"
                    "=== 支持的运算 ===\n"
                    "+ - * / : 四则运算\n"
                    "<< >>   : 位移运算\n"
                    "& | ^   : 位运算\n"
                    "sin, cos, sqrt, abs 等数学函数"
                );
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 按钮
            if (ImGui::Button("确定", ImVec2(100, 0))) {
                entry.alias = m_aliasBuffer;
                entry.formula = m_readFormulaInput;
                entry.writeFormula = m_writeFormulaInput;
                entry.parseConfigured = !entry.formula.empty();

                // 立即更新解析值
                if (entry.parseConfigured) {
                    m_viewModel->UpdateParsedValue(m_parseEditIndex);
                }

                m_showParsePopup = false;
            }

            ImGui::SameLine();

            if (ImGui::Button("取消", ImVec2(100, 0))) {
                m_showParsePopup = false;
            }

            ImGui::SameLine();

            if (ImGui::Button("清除配置", ImVec2(100, 0))) {
                entry.alias.clear();
                entry.formula.clear();
                entry.writeFormula.clear();
                entry.parseConfigured = false;
                entry.showCurve = false;
                m_showParsePopup = false;
            }

            ImGui::EndPopup();
        }
    }

    // 打开解析配置弹窗
    void I2CTableWindow::OpenParseConfigPopup(int entryIndex) {
        m_parseConfigEntryIndex = entryIndex;
        auto& config = m_viewModel->GetParseConfig(entryIndex);
        strncpy(m_readFormulaInput, config.readFormula.c_str(), sizeof(m_readFormulaInput) - 1);
        strncpy(m_writeFormulaInput, config.writeFormula.c_str(), sizeof(m_writeFormulaInput) - 1);
        m_showParseConfigPopup = true;

    }
    void I2CTableWindow::RenderPropertyPopup()
    {
        if (m_showPropertyPopup) {
            ImGui::OpenPopup("命令属性");
        }

        if (ImGui::BeginPopupModal("命令属性", &m_showPropertyPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Checkbox("使能覆写从机地址", &m_propertyOverride);
            if (m_propertyOverride) {
                ImGui::Text("从机地址:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputText("##propSlaveAddr", m_propertySlaveAddr, sizeof(m_propertySlaveAddr));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("确定", ImVec2(80, 0))) {
                auto& group = m_viewModel->GetCurrentGroup1();
                uint8_t addr = m_viewModel->ParseHexInput(m_propertySlaveAddr);
                if (m_propertyTabType == 0 && m_propertyEditIndex < static_cast<int>(group.registerEntries.size())) {
                    group.registerEntries[m_propertyEditIndex].overrideSlaveAddr = m_propertyOverride;
                    group.registerEntries[m_propertyEditIndex].slaveAddress = addr;
                }
                else if (m_propertyTabType == 1 && m_propertyEditIndex < static_cast<int>(group.singleTriggerEntries.size())) {
                    group.singleTriggerEntries[m_propertyEditIndex].overrideSlaveAddr = m_propertyOverride;
                    group.singleTriggerEntries[m_propertyEditIndex].slaveAddress = addr;
                }
                else if (m_propertyTabType == 2 && m_propertyEditIndex < static_cast<int>(group.periodicTriggerEntries.size())) {
                    group.periodicTriggerEntries[m_propertyEditIndex].overrideSlaveAddr = m_propertyOverride;
                    group.periodicTriggerEntries[m_propertyEditIndex].slaveAddress = addr;
                }

                m_showPropertyPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showPropertyPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::RenderButtonNamePopup()
    {
        if (m_showButtonNamePopup) {
            ImGui::OpenPopup("设置按钮名称");
        }

        if (ImGui::BeginPopup("设置按钮名称")) {
            ImGui::Text("按钮名称:");
            ImGui::InputText("##btnname", m_buttonNameBuffer, sizeof(m_buttonNameBuffer));
            if (ImGui::Button("确定")) {
                auto& group = m_viewModel->GetCurrentGroup1();

                if (m_buttonNameTabType == 1 && m_buttonNameEditIndex < static_cast<int>(group.singleTriggerEntries.size())) {
                    group.singleTriggerEntries[m_buttonNameEditIndex].buttonName = m_buttonNameBuffer;
                }
                else if (m_buttonNameTabType == 2 && m_buttonNameEditIndex < static_cast<int>(group.periodicTriggerEntries.size())) {
                    group.periodicTriggerEntries[m_buttonNameEditIndex].buttonName = m_buttonNameBuffer;
                }

                m_showButtonNamePopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消")) {
                m_showButtonNamePopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::RenderRenamePopup()
    {
        if (m_showRenamePopup) {
            ImGui::OpenPopup("重命名命令组");
        }

        if (ImGui::BeginPopupModal("重命名命令组", &m_showRenamePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("新名称:");
            ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer));

            ImGui::Spacing();

            if (ImGui::Button("确定", ImVec2(80, 0))) {
                m_viewModel->RenameGroup(m_renameBuffer);
                m_showRenamePopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showRenamePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::RenderParsePopup()
    {
        if (m_showParsePopup) {
            ImGui::OpenPopup("解析设置");
        }

        if (ImGui::BeginPopupModal("解析设置", &m_showParsePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("别名:");
            ImGui::InputText("##alias", m_aliasBuffer, sizeof(m_aliasBuffer));

            ImGui::Text("解析公式:");
            ImGui::InputText("##formula", m_readFormulaInput, sizeof(m_readFormulaInput));

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "变量说明: b0,b1,b2...代表读取的第1,2,3个字节\n"
                "w0代表(b1<<8)|b0(小端字)");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("确定", ImVec2(80, 0))) {
                auto& entries = m_viewModel->GetCurrentGroup1().periodicTriggerEntries;
                if (m_parseEditIndex < static_cast<int>(entries.size())) {
                    entries[m_parseEditIndex].alias = m_aliasBuffer;
                    entries[m_parseEditIndex].formula = m_readFormulaInput;
                    entries[m_parseEditIndex].parseConfigured =
                        (strlen(m_aliasBuffer) > 0 || strlen(m_readFormulaInput) > 0);
                }
                m_showParsePopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(80, 0))) {
                m_showParsePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void I2CTableWindow::SyncInputBuffersFromModel() {
        auto& group = m_viewModel->GetCurrentGroup();

            // 同步从机地址输入框
            snprintf(m_slaveAddrInput, sizeof(m_slaveAddrInput), "%02X", group.slaveAddress);

        // 同步间隔输入框
        snprintf(m_intervalInput, sizeof(m_intervalInput), "%u", group.interval);
    }

    void I2CTableWindow::InitializeInputBuffers() {
        SyncInputBuffersFromModel();
    }
}

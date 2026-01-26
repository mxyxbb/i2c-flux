#include "i2c_simple_window.h"
#include "../../viewmodels/i2c_simple_viewmodel.h"
#include "imgui.h"
#include <cstdio>

namespace I2CDebugger {

I2CSimpleWindow::I2CSimpleWindow(std::shared_ptr<I2CSimpleViewModel> viewModel)
    : m_viewModel(viewModel)
{
}

//绘制活动指示灯
static void DrawActivityIndicator(ActivityIndicator& indicator) {
    bool isOn = indicator.Update();
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    float radius = 6.0f;
    ImVec2 center(pos.x + radius + 2, pos.y + 3 +ImGui::GetTextLineHeight() / 2);
    
    if (isOn) {
        //亮灯 - 绿色
        drawList->AddCircleFilled(center, radius, IM_COL32(0, 255, 0, 255));
        drawList->AddCircle(center, radius, IM_COL32(0, 200, 0, 255), 0, 2.0f);
    } else {
        // 灭灯 - 灰色
        drawList->AddCircleFilled(center, radius, IM_COL32(80, 80, 80, 255));
        drawList->AddCircle(center, radius, IM_COL32(60, 60, 60, 255), 0, 1.0f);
    }
    // 占位
    ImGui::Dummy(ImVec2(radius * 2 + 4, ImGui::GetTextLineHeight()));
}

void I2CSimpleWindow::Render(bool* p_open)
{
    auto& data = m_viewModel->GetData();

    ImGui::SetNextWindowSize(ImVec2(450, 500), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("简单操作窗口", p_open)) {  // 使用 p_open 参数
        ImGui::End();
        return;
    }

    RenderDeviceConnection();
    ImGui::Separator();
    RenderSlaveScanner();
    ImGui::Separator();
    RenderSimpleOperation();

    ImGui::End();
}

void I2CSimpleWindow::RenderDeviceConnection()
{
    auto& data = m_viewModel->GetData();
    
    ImGui::Text("设备连接");
    ImGui::Spacing();
    
    // 设备名称
    ImGui::Text("设备: %s", data.deviceName.empty() ? "未连接" : data.deviceName.c_str());
    
    // 波特率选择
    ImGui::Text("波特率:");
    ImGui::SameLine();
    const char* baudItems[] = { "100K", "400K" };
    int baudIndex = (data.baudRate == BAUD_RATE_400K) ? 1 : 0;
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("##Baud", &baudIndex, baudItems, 2)) {
        data.baudRate = (baudIndex == 1) ? BAUD_RATE_400K : BAUD_RATE_100K;
    }
    
    // 连接按钮
    ImGui::SameLine();
    if (data.isConnected) {
        if (ImGui::Button("断开设备", ImVec2(100, 0))) {
            m_viewModel->Disconnect();
        }
    } else {
        if (ImGui::Button("连接设备", ImVec2(100, 0))) {
            m_viewModel->Connect();
        }
    }
    
    // 连接状态
    ImGui::SameLine();
    if (data.isConnected) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "已连接");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "未连接");
    }
}

void I2CSimpleWindow::RenderSlaveScanner()
{
    auto& data = m_viewModel->GetData();
    
    ImGui::Text("从机扫描");
    ImGui::Spacing();
    
    // 扫描按钮
    if (data.isScanning) {
        ImGui::BeginDisabled();
        ImGui::Button("扫描中...", ImVec2(100, 0));
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button("扫描从机", ImVec2(100, 0))) {
            m_viewModel->ScanSlaves();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("扫描I2C总线上的从机设备\n地址范围: 0x02-0x7F (7位地址)");
    }
    
    // 扫描结果表格
    ImGui::Spacing();
    
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
    
    if (ImGui::BeginTable("SlaveTable", 3, flags, ImVec2(0, 80))) {
        ImGui::TableSetupColumn("序号", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("从机地址", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("选择", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableHeadersRow();
        
        for (int i = 0; i < 3; i++) {
            ImGui::TableNextRow();
            
            if (i < static_cast<int>(data.scannedSlaves.size())) {
                auto& slave = data.scannedSlaves[i];
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", i + 1);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("0x%02X", slave.address);
                
                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(i);
                if (ImGui::RadioButton("##sel", slave.selected)) {
                    m_viewModel->SelectSlave(i);
                }
                ImGui::PopID();
            } else {
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("-");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("-");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextDisabled("-");
            }
        }
        
        ImGui::EndTable();
    }
}

void I2CSimpleWindow::RenderSimpleOperation()
{
    auto& data = m_viewModel->GetData();
    
    ImGui::Text("简单操作");
    ImGui::Spacing();
    
    // 从机地址
    ImGui::Text("从机地址:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##SlaveAddr", data.slaveAddrInput, sizeof(data.slaveAddrInput));
    
    // 寄存器地址
    ImGui::SameLine();
    ImGui::Text("寄存器地址:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##RegAddr", data.regAddrInput, sizeof(data.regAddrInput));
    
    // 操作类型
    ImGui::Text("操作类型:");
    ImGui::SameLine();
    const char* opItems[] = { "读取", "写入", "发命令" };
    int opIndex = static_cast<int>(data.operationType);
    ImGui::SetNextItemWidth(100);
    if (ImGui::Combo("##OpType", &opIndex, opItems, 3)) {
        data.operationType = static_cast<OperationType>(opIndex);
    }
    
    // 根据操作类型显示不同内容
    switch (data.operationType) {
        case OperationType::Read:
            ImGui::Text("读取长度:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputText("##Length", data.lengthInput, sizeof(data.lengthInput));
            ImGui::Text("读取结果:");
            ImGui::SameLine();
            {
                std::string result = m_viewModel->FormatHexData(data.readData);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s", result.empty() ? "-" : result.c_str());
            }
            break;
        case OperationType::Write:
            ImGui::Text("写入数据:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##WriteData", data.writeDataInput, sizeof(data.writeDataInput));
            ImGui::SameLine();
            ImGui::TextDisabled("(空格分隔)");
            break;
            
        case OperationType::SendCommand:
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "提示: 寄存器地址即为命令代码");
            break;
    }
    
    ImGui::Spacing();
    
    // 执行按钮
    if (data.isOperating) {
        ImGui::BeginDisabled();
        ImGui::Button("执行中...", ImVec2(100, 0));
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button("执行操作", ImVec2(100, 0))) {
            m_viewModel->ExecuteOperation();
        }
    }
    
    // 活动指示灯
    ImGui::SameLine();
    DrawActivityIndicator(data.activityIndicator);
    
    // 操作结果
    ImGui::Spacing();
    if (!data.lastOperationSuccess && !data.lastErrorMessage.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "错误: %s", data.lastErrorMessage.c_str());
    }
}

}

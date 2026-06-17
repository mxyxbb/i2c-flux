#include "i2c_simple_window.h"
#include "../../viewmodels/i2c_simple_viewmodel.h"
#include "imgui.h"
#include <cstdio>

namespace I2CDebugger {

I2CSimpleWindow::I2CSimpleWindow(std::shared_ptr<I2CSimpleViewModel> viewModel)
    : m_viewModel(viewModel)
{
}

void I2CSimpleWindow::RenderTooltip(const char* text)
{
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
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

    // 波特率选择（以 K 为单位输入，内部显示K）
    ImGui::Text("波特率:");
    ImGui::SameLine();

    // 1. 创建临时变量，将 Hz 转换为 kHz 用于界面操作
    uint32_t baudRateK = data.baudRate / 1000;

    // 稍微加宽一点，确保能完整显示 "400 K"
    ImGui::SetNextItemWidth(70);

    // 2. 核心修改：格式化字符串改为 "%u K"。
    // 这样界面上会显示如 "400 K"，且用户修改数字后，ImGui 依然能正确解析出 400
    if (ImGui::InputScalar("##BaudInput", ImGuiDataType_U32, &baudRateK, nullptr, nullptr, "%uK")) {
        // 用户修改后，存回底层时乘以 1000 还原为 Hz
        data.baudRate = baudRateK * 1000;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("输入自定义波特率 (kHz)\n直接输入数字即可，'K' 会自动保留");
    }

    // 3. 紧贴输入框添加下拉箭头按钮
    ImGui::SameLine(0, 0);
    if (ImGui::ArrowButton("##BaudPresetsBtn", ImGuiDir_Down)) {
        ImGui::OpenPopup("BaudPresetsPopup");
    }

    // 4. 渲染预设选项的弹窗
    if (ImGui::BeginPopup("BaudPresetsPopup")) {
        if (ImGui::Selectable("100K"))  data.baudRate = 100000;
        if (ImGui::Selectable("400K"))  data.baudRate = 400000;
        if (ImGui::Selectable("1000K (1M)")) data.baudRate = 1000000;
        ImGui::EndPopup();
    }

    // 连接按钮
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(5, 0)); // 加一点间距，防止按钮粘连
    ImGui::SameLine();
    if (data.isConnected) {
        if (ImGui::Button("断开设备", ImVec2(100, 0))) {
            m_viewModel->Disconnect();
        }
    }
    else {
        if (ImGui::Button("连接设备", ImVec2(100, 0))) {
            m_viewModel->Connect();
        }
        RenderTooltip("点击后自动扫描设备:\n\r1. CH347T\n\r2. CP2112\n\r3. NI-845x\n\r4. RPI2C");
    }

    // 连接状态
    ImGui::SameLine();
    if (data.isConnected) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "已连接");
    }
    else {
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
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("16进制数据，带不带0x均可");
    }
    // 寄存器地址
    ImGui::SameLine();
    ImGui::Text("寄存器地址:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##RegAddr", data.regAddrInput, sizeof(data.regAddrInput));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("16进制数据，带不带0x均可");
    }

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
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("10进制数，CP2112最大511");
            }
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
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("16进制数，CP2112最大511个字节");
            }
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

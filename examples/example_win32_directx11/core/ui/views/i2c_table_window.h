#pragma once
#include "../../viewmodels/i2c_table_viewmodel.h"
#include <memory>
#include <chrono>  // 添加这个头文件

namespace I2CDebugger {

    class I2CTableViewModel;

    class I2CTableWindow {
    public:
        explicit I2CTableWindow(std::shared_ptr<I2CTableViewModel> viewModel);

        void Render(bool* p_open = nullptr);
        // 初始化输入框缓冲区（在配置加载后调用）
        void InitializeInputBuffers();

    private:
        void RenderGroupSelector();
        void RenderSlaveAddressInput();
        void RenderRegisterTableTab();
        void RenderSingleTriggerTab();
        void RenderPeriodicTriggerTab();
        void RenderPropertyPopup();
        void RenderButtonNamePopup();
        void RenderRenamePopup();
        void RenderExportPopup();
        void RenderImportPopup();
        // 同步输入框缓冲区与数据模型
        void SyncInputBuffersFromModel();
        void RenderLogControls();  // 添加这一行声明
        // 渲染解析配置弹窗
        void RenderParseConfigPopup();
        void OpenParseConfigPopup(int entryIndex);
        void RenderRegisterParsePopup();        // 新增：寄存器表解析弹窗
        void RenderSingleParsePopup();          // 新增：单次触发解析弹窗
        void RenderLogSettingsPopup();          // 新增：日志设置弹窗
        void RenderDataLogSettingsPopup();

        std::shared_ptr<I2CTableViewModel> m_viewModel;

        // 输入缓冲区
        char m_slaveAddrInput[16] = "0x50";
        char m_intervalInput[16] = "100";

        // 弹窗状态
        bool m_showPropertyPopup = false;
        bool m_showButtonNamePopup = false;
        bool m_showRenamePopup = false;
        bool m_showParsePopup = false;
        bool m_showExportPopup = false;
        bool m_showImportPopup = false;

        //弹窗编辑数据
        int m_propertyEditIndex = -1;
        int m_propertyTabType = 0;
        bool m_propertyOverride = false;
        char m_propertySlaveAddr[16] = "0x50";

        int m_buttonNameEditIndex = -1;
        int m_buttonNameTabType = 0;
        char m_buttonNameBuffer[64] = "";

        char m_renameBuffer[128] = "";

        int m_parseEditIndex = -1;
        //char m_aliasBuffer[64] = "";
        //char m_formulaBuffer[256] = "";

        // 导出/导入路径缓冲区
        char m_exportPathBuffer[512] = "";
        char m_importPathBuffer[512] = "";

        // 解析配置弹窗相关
        bool m_showParseConfigPopup = false;
        int m_parseConfigEntryIndex = -1;
        char m_aliasBuffer[64] = { 0 };
        char m_readFormulaInput[256] = { 0 };
        char m_writeFormulaInput[256] = { 0 };  // 写入公式缓冲区
        char m_parsedValueBuffer[64] = { 0 };    // 解析值编辑缓冲区



        // 日志设置弹窗
        bool m_showLogSettingsPopup = false;
        //char m_logFilePathBuffer[512] = "";

        // 寄存器表解析弹窗
        bool m_showRegisterParsePopup = false;
        int m_registerParseEditIndex = -1;

        // 单次触发解析弹窗
        bool m_showSingleParsePopup = false;
        int m_singleParseEditIndex = -1;

        // 数据记录设置弹窗
        bool m_showDataLogSettingsPopup = false;
        char m_logFilePathBuffer[512] = "";

        // 辅助函数
        std::string GenerateCSVHeaderPreview();
    };

}

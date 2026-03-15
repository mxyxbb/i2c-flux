#pragma once
#include <memory>
#include <cstring>
#include "../../viewmodels/crc_viewmodel.h"

namespace I2CDebugger {
    class CrcView {
    public:
        CrcView() {
            std::memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
            std::strncpy(m_simSlaveAddr, "0x50", sizeof(m_simSlaveAddr));
            std::strncpy(m_simRegAddr, "0x00", sizeof(m_simRegAddr));
            std::strncpy(m_simLen, "1", sizeof(m_simLen));
            std::strncpy(m_simData, "00", sizeof(m_simData));
        }

        void Open(std::shared_ptr<CrcViewModel> viewModel);
        void Render(std::shared_ptr<CrcViewModel> viewModel);

    private:
        bool m_showPopup = false;
        char m_inputBuffer[256];

        // --- 新增：模拟读写的 UI 状态 ---
        char m_simSlaveAddr[16];
        int  m_simRwMode = 0; // 0: 写, 1: 读
        char m_simRegAddr[16];
        char m_simLen[16];
        char m_simData[256];
    };
}

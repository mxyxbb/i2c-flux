#pragma once
#include <memory>

namespace I2CDebugger {

    class I2CTableViewModel;

    class QuickAddPopup {
    public:
        QuickAddPopup() = default;
        ~QuickAddPopup() = default;

        void Open(int tabType);
        void Render(std::shared_ptr<I2CTableViewModel> viewModel);

    private:
        bool m_show = false;
        int m_tabType = 0;
        char m_buffer[8192] = "";
    };

} // namespace I2CDebugger

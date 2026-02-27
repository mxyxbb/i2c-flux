#pragma once
#include <string>
#include <memory>

namespace I2CDebugger {
    // 前向声明，避免包含过多的头文件
    class I2CTableViewModel;

    class QuickAddProcessor {
    public:
        // 静态方法：处理快捷添加的逻辑
        static void Process(std::shared_ptr<I2CTableViewModel> viewModel, int tabType, const std::string& csvData);
    };

} // namespace I2CDebugger

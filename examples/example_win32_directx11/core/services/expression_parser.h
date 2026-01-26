#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// ExprTK 编译较慢，使用前向声明
namespace exprtk {
    template <typename T> class expression;
    template <typename T> class symbol_table;
    template <typename T> class parser;
}

namespace I2CDebugger {

    // 解析结果
    struct ParseResult {
        bool success = false;
        double value = 0.0;
        std::string errorMsg;
    };

    class ExpressionParser {
    public:
        ExpressionParser();
        ~ExpressionParser();

        // 使用读取公式将原始字节转换为十进制值
        // formula: 公式字符串，例如 "(b1 << 8) | b0"
        // rawData: 原始字节数据
        ParseResult EvaluateReadFormula(const std::string& formula,
            const std::vector<uint8_t>& rawData);

        // 使用写入公式将十进制值转换为原始字节
        // formula: 公式字符串
        // value: 十进制值
        // byteCount: 输出字节数
        std::vector<uint8_t> EvaluateWriteFormula(const std::string& formula,
            double value,
            size_t byteCount,
            bool& success,
            std::string& errorMsg);

        // 验证公式是否有效
        bool ValidateFormula(const std::string& formula, std::string& errorMsg);

        // 获取公式帮助文本
        static std::string GetFormulaHelp();

    private:
        // 设置字节变量 b0, b1, b2... 和 w0, w1...
        void SetByteVariables(const std::vector<uint8_t>& rawData);

        // 字节变量数组 (最多支持32字节)
        double m_bytes[32] = { 0 };
        double m_words[16] = { 0 };  // 小端字
        double m_value = 0;        // 用于写入公式的输入值
        double m_result = 0;       // 结果变量
    };

}

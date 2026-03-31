#include "expression_parser.h"
#include <regex> // 【新增】：用于正则替换十六进制

// 禁用一些警告，ExprTK 头文件较大
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267 4996)
#endif

#include "../exprtk/exprtk.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ==========================================
// 【新增】：自定义位运算与预处理函数
// ==========================================
namespace {
    struct BitwiseAnd : public exprtk::ifunction<double> {
        BitwiseAnd() : exprtk::ifunction<double>(2) {}
        inline double operator()(const double& v1, const double& v2) {
            return static_cast<double>(static_cast<uint64_t>(v1) & static_cast<uint64_t>(v2));
        }
    };
    struct BitwiseOr : public exprtk::ifunction<double> {
        BitwiseOr() : exprtk::ifunction<double>(2) {}
        inline double operator()(const double& v1, const double& v2) {
            return static_cast<double>(static_cast<uint64_t>(v1) | static_cast<uint64_t>(v2));
        }
    };
    struct BitwiseXor : public exprtk::ifunction<double> {
        BitwiseXor() : exprtk::ifunction<double>(2) {}
        inline double operator()(const double& v1, const double& v2) {
            return static_cast<double>(static_cast<uint64_t>(v1) ^ static_cast<uint64_t>(v2));
        }
    };
    struct BitwiseNot : public exprtk::ifunction<double> {
        BitwiseNot() : exprtk::ifunction<double>(1) {}
        inline double operator()(const double& v) {
            return static_cast<double>(~static_cast<uint64_t>(v));
        }
    };
    struct ShiftLeft : public exprtk::ifunction<double> {
        ShiftLeft() : exprtk::ifunction<double>(2) {}
        inline double operator()(const double& v1, const double& v2) {
            return static_cast<double>(static_cast<uint64_t>(v1) << static_cast<uint64_t>(v2));
        }
    };
    struct ShiftRight : public exprtk::ifunction<double> {
        ShiftRight() : exprtk::ifunction<double>(2) {}
        inline double operator()(const double& v1, const double& v2) {
            return static_cast<double>(static_cast<uint64_t>(v1) >> static_cast<uint64_t>(v2));
        }
    };

    void RegisterBitwiseFunctions(exprtk::symbol_table<double>& symbolTable) {
        static BitwiseAnd op_and;
        static BitwiseOr  op_or;
        static BitwiseXor op_xor;
        static BitwiseNot op_not;
        static ShiftLeft  op_shl;
        static ShiftRight op_shr;

        symbolTable.add_function("band", op_and);
        symbolTable.add_function("bor", op_or);
        symbolTable.add_function("bxor", op_xor);
        symbolTable.add_function("bnot", op_not);
        symbolTable.add_function("shl", op_shl);
        symbolTable.add_function("shr", op_shr);
    }

    // 【新增】：预处理公式，将 0x 或 0X 开头的十六进制数替换为十进制字符串
    std::string PreprocessHex(const std::string& formula) {
        // \b 确保只匹配独立的数值，不会误伤名为 x0xA 的变量名
        std::regex hex_regex(R"(\b0[xX][0-9a-fA-F]+\b)");
        std::smatch match;
        std::string result;
        std::string::const_iterator searchStart(formula.cbegin());

        while (std::regex_search(searchStart, formula.cend(), match, hex_regex)) {
            result += match.prefix();
            try {
                // 将截获到的 16 进制字符串转为标准的 10 进制无符号数
                unsigned long long val = std::stoull(match.str(), nullptr, 16);
                result += std::to_string(val);
            }
            catch (...) {
                // 如果发生越界等异常，原样保留以交由后续报错
                result += match.str();
            }
            searchStart = match.suffix().first;
        }
        result += std::string(searchStart, formula.cend());
        return result;
    }
}

namespace I2CDebugger {

    ExpressionParser::ExpressionParser() {
        for (int i = 0; i < 32; ++i) m_bytes[i] = 0;
        for (int i = 0; i < 16; ++i) m_words[i] = 0;
    }

    ExpressionParser::~ExpressionParser() = default;

    void ExpressionParser::SetByteVariables(const std::vector<uint8_t>& rawData) {
        for (int i = 0; i < 32; ++i) m_bytes[i] = 0;
        for (int i = 0; i < 16; ++i) m_words[i] = 0;

        for (size_t i = 0; i < rawData.size() && i < 32; ++i) {
            m_bytes[i] = static_cast<double>(rawData[i]);
        }

        for (size_t i = 0; i < 16 && (i * 2 + 1) < rawData.size(); ++i) {
            m_words[i] = static_cast<double>((rawData[i * 2 + 1] << 8) | rawData[i * 2]);
        }
    }

    ParseResult ExpressionParser::EvaluateReadFormula(const std::string& formula,
        const std::vector<uint8_t>& rawData) {
        ParseResult result;

        if (formula.empty()) {
            result.errorMsg = "公式为空";
            return result;
        }

        if (rawData.empty()) {
            result.errorMsg = "数据为空";
            return result;
        }

        if (formula == "str" || formula == "string") {
            result.isString = true;
            std::string parsedStr;
            for (uint8_t byte : rawData) {
                if (byte == 0x00) break;
                parsedStr.push_back(static_cast<char>(byte));
            }
            result.stringValue = parsedStr;
            result.success = true;
            return result;
        }

        SetByteVariables(rawData);

        exprtk::symbol_table<double> symbolTable;
        RegisterBitwiseFunctions(symbolTable);

        for (int i = 0; i < 32; ++i) {
            std::string varName = "b" + std::to_string(i);
            symbolTable.add_variable(varName, m_bytes[i]);
        }

        for (int i = 0; i < 16; ++i) {
            std::string varName = "w" + std::to_string(i);
            symbolTable.add_variable(varName, m_words[i]);
        }

        symbolTable.add_constants();

        exprtk::expression<double> expression;
        expression.register_symbol_table(symbolTable);

        // 【新增】：在编译前过滤掉十六进制字符
        std::string processedFormula = PreprocessHex(formula);

        exprtk::parser<double> parser;
        if (!parser.compile(processedFormula, expression)) {
            result.errorMsg = "公式解析错误: " + parser.error();
            return result;
        }

        result.value = expression.value();
        result.isString = false;
        result.success = true;
        return result;
    }

    std::vector<uint8_t> ExpressionParser::EvaluateWriteFormula(const std::string& formula,
        double value,
        size_t byteCount,
        bool& success,
        std::string& errorMsg) {
        std::vector<uint8_t> result;
        success = false;

        if (formula.empty()) {
            int64_t intValue = static_cast<int64_t>(value);
            for (size_t i = 0; i < byteCount; ++i) {
                result.push_back(static_cast<uint8_t>((intValue >> (i * 8)) & 0xFF));
            }
            success = true;
            return result;
        }

        if (formula == "str" || formula == "string") {
            errorMsg = "字符串模式不支持数值输入";
            return result;
        }

        exprtk::symbol_table<double> symbolTable;
        RegisterBitwiseFunctions(symbolTable);

        m_value = value;
        symbolTable.add_variable("value", m_value);
        symbolTable.add_variable("v", m_value);
        symbolTable.add_constants();

        exprtk::expression<double> expression;
        expression.register_symbol_table(symbolTable);

        // 【新增】：在编译前过滤掉十六进制字符
        std::string processedFormula = PreprocessHex(formula);

        exprtk::parser<double> parser;
        if (!parser.compile(processedFormula, expression)) {
            errorMsg = "公式解析错误: " + parser.error();
            return result;
        }

        double rawValue = expression.value();
        int64_t intValue = static_cast<int64_t>(rawValue);

        for (size_t i = 0; i < byteCount; ++i) {
            result.push_back(static_cast<uint8_t>((intValue >> (i * 8)) & 0xFF));
        }

        success = true;
        return result;
    }

    std::vector<uint8_t> ExpressionParser::EvaluateWriteFormula(const std::string& formula,
        const std::string& strValue,
        size_t byteCount,
        bool& success,
        std::string& errorMsg) {
        std::vector<uint8_t> result;
        success = false;

        if (formula == "str" || formula == "string") {
            for (size_t i = 0; i < byteCount; ++i) {
                if (i < strValue.length()) {
                    result.push_back(static_cast<uint8_t>(strValue[i]));
                }
                else {
                    result.push_back(0x00);
                }
            }
            success = true;
            return result;
        }

        try {
            double numValue = std::stod(strValue);
            return EvaluateWriteFormula(formula, numValue, byteCount, success, errorMsg);
        }
        catch (...) {
            errorMsg = "无效的输入格式";
            return result;
        }
    }

    bool ExpressionParser::ValidateFormula(const std::string& formula, std::string& errorMsg) {
        if (formula.empty()) {
            errorMsg = "公式为空";
            return false;
        }

        if (formula == "str" || formula == "string") {
            return true;
        }

        exprtk::symbol_table<double> symbolTable;
        RegisterBitwiseFunctions(symbolTable);

        double dummyBytes[32] = { 0 };
        double dummyWords[16] = { 0 };
        double dummyValue = 0;

        for (int i = 0; i < 32; ++i) {
            symbolTable.add_variable("b" + std::to_string(i), dummyBytes[i]);
        }
        for (int i = 0; i < 16; ++i) {
            symbolTable.add_variable("w" + std::to_string(i), dummyWords[i]);
        }
        symbolTable.add_variable("value", dummyValue);
        symbolTable.add_variable("v", dummyValue);
        symbolTable.add_constants();

        exprtk::expression<double> expression;
        expression.register_symbol_table(symbolTable);

        // 【新增】：在编译前过滤掉十六进制字符
        std::string processedFormula = PreprocessHex(formula);

        exprtk::parser<double> parser;
        if (!parser.compile(processedFormula, expression)) {
            errorMsg = parser.error();
            return false;
        }

        return true;
    }

    std::string ExpressionParser::GetFormulaHelp() {
        return
            "=== 公式变量说明 ===\n"
            "读取公式变量:\n"
            "  b0, b1, b2... : 第1, 2, 3...个字节\n"
            "  w0, w1...     : 小端字, w0 = (b1<<8)|b0\n"
            "\n"
            "写入公式变量:\n"
            "  value 或 v    : 输入的十进制解析值\n"
            "\n"
            "特殊公式:\n"
            "  str   : 将数据视为ASCII字符串进行解析或写入\n"
            "\n"
            "=== 位运算与扩展函数 ===\n"
            "ExprTK 解析器支持以下函数运算，且原生支持 0x 十六进制语法：\n"
            "  band(a, b) : 按位与 (a & b)\n"
            "  bor(a, b)  : 按位或 (a | b)\n"
            "  bxor(a, b) : 按位异或 (a ^ b)\n"
            "  bnot(a)    : 按位取反 (~a)\n"
            "  shl(a, b)  : 左移 (a << b)\n"
            "  shr(a, b)  : 右移 (a >> b)\n"
            "\n"
            "=== 公式示例 ===\n"
            "读取公式:\n"
            "  w0                  : 2字节小端转整数\n"
            "  w0/32               : 2字节小端Q5转整数\n"
            "  bor(shl(b1,8), b0)  : 相当于 (b1 << 8) | b0\n"
            "  band(b0, 0x7F)      : 屏蔽 b0 的最高位 (支持0x格式)\n"
            "  str                 : 解析为文本字符串\n"
            "\n"
            "写入公式:\n"
            "  v * 100             : 输入值乘以100\n"
            "  band(v, 0xFF)       : 取输入的低8位\n"
            "  str                 : 直接写入ASCII字符\n";
    }

}

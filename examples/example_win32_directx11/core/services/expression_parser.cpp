#include "expression_parser.h"

// 禁用一些警告，ExprTK 头文件较大
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267 4996)
#endif

#include "../exprtk/exprtk.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace I2CDebugger {

    ExpressionParser::ExpressionParser() {
        // 初始化变量数组
        for (int i = 0; i < 32; ++i) m_bytes[i] = 0;
        for (int i = 0; i < 16; ++i) m_words[i] = 0;
    }

    ExpressionParser::~ExpressionParser() = default;

    void ExpressionParser::SetByteVariables(const std::vector<uint8_t>& rawData) {
        // 清零
        for (int i = 0; i < 32; ++i) m_bytes[i] = 0;
        for (int i = 0; i < 16; ++i) m_words[i] = 0;

        // 设置字节变量
        for (size_t i = 0; i < rawData.size() && i < 32; ++i) {
            m_bytes[i] = static_cast<double>(rawData[i]);
        }

        // 设置小端字变量 w0 = (b1 << 8) | b0
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

        // ==========================================
        // 【新增】：字符串解析模式
        // ==========================================
        if (formula == "str" || formula == "string") {
            result.isString = true;
            std::string parsedStr;
            for (uint8_t byte : rawData) {
                if (byte == 0x00) break; // 遇到字符串结束符 \0 停止
                parsedStr.push_back(static_cast<char>(byte));
            }
            result.stringValue = parsedStr;
            result.success = true;
            return result;
        }

        // 设置字节变量
        SetByteVariables(rawData);

        // 创建符号表
        exprtk::symbol_table<double> symbolTable;

        // 注册字节变量 b0-b31
        for (int i = 0; i < 32; ++i) {
            std::string varName = "b" + std::to_string(i);
            symbolTable.add_variable(varName, m_bytes[i]);
        }

        // 注册小端字变量 w0-w15
        for (int i = 0; i < 16; ++i) {
            std::string varName = "w" + std::to_string(i);
            symbolTable.add_variable(varName, m_words[i]);
        }

        // 添加常用常量和函数
        symbolTable.add_constants();

        // 创建表达式
        exprtk::expression<double> expression;
        expression.register_symbol_table(symbolTable);

        // 解析公式
        exprtk::parser<double> parser;
        if (!parser.compile(formula, expression)) {
            result.errorMsg = "公式解析错误: " + parser.error();
            return result;
        }

        // 计算结果
        result.value = expression.value();
        result.isString = false;
        result.success = true;
        return result;
    }

    // 数值写入模式（原有函数保持不变）
    std::vector<uint8_t> ExpressionParser::EvaluateWriteFormula(const std::string& formula,
        double value,
        size_t byteCount,
        bool& success,
        std::string& errorMsg) {
        std::vector<uint8_t> result;
        success = false;

        if (formula.empty()) {
            // 默认行为：直接将值转换为字节（小端序）
            int64_t intValue = static_cast<int64_t>(value);
            for (size_t i = 0; i < byteCount; ++i) {
                result.push_back(static_cast<uint8_t>((intValue >> (i * 8)) & 0xFF));
            }
            success = true;
            return result;
        }

        // 防御：如果是 str 模式，但误调了 double 接口
        if (formula == "str" || formula == "string") {
            errorMsg = "字符串模式不支持数值输入";
            return result;
        }

        // 创建符号表
        exprtk::symbol_table<double> symbolTable;
        m_value = value;
        symbolTable.add_variable("value", m_value);
        symbolTable.add_variable("v", m_value);  // 简写
        symbolTable.add_constants();

        // 创建表达式
        exprtk::expression<double> expression;
        expression.register_symbol_table(symbolTable);

        // 解析公式
        exprtk::parser<double> parser;
        if (!parser.compile(formula, expression)) {
            errorMsg = "公式解析错误: " + parser.error();
            return result;
        }

        // 计算结果
        double rawValue = expression.value();
        int64_t intValue = static_cast<int64_t>(rawValue);

        // 转换为字节数组（小端序）
        for (size_t i = 0; i < byteCount; ++i) {
            result.push_back(static_cast<uint8_t>((intValue >> (i * 8)) & 0xFF));
        }

        success = true;
        return result;
    }

    // ==========================================
    // 【新增】：字符串写入模式重载
    // 专门用于处理 string -> Raw Bytes 的转换
    // ==========================================
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
                    result.push_back(0x00); // 如果目标长度大于字符串长度，补 0
                }
            }
            success = true;
            return result;
        }

        // 如果并非 str 模式但调用了此接口，尝试将字符串转为 double 交给原函数处理
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

        // 【新增】：支持 str 常规校验
        if (formula == "str" || formula == "string") {
            return true;
        }

        // 创建符号表，添加所有可能的变量
        exprtk::symbol_table<double> symbolTable;

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

        exprtk::parser<double> parser;
        if (!parser.compile(formula, expression)) {
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
            "  str   : 将数据视为ASCII字符串进行解析或写入\n" // 【新增】说明
            "\n"
            "=== 公式示例 ===\n"
            "读取公式:\n"
            "  w0        : 2字节小端转整数\n"
            "  w0/32     : 2字节小端Q5转整数\n"
            "  str       : 解析为文本字符串\n" // 【新增】示例
            "\n"
            "写入公式:\n"
            "  v * 100   : 输入值乘以100\n"
            "  str       : 直接写入ASCII字符\n";
    }

}

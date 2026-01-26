// core/utils/expression_evaluator.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../ExprTK/exprtk.hpp"
#include "../models/i2c_command.h"
#include <algorithm> // for std::reverse

class ExpressionEvaluator {
public:
    struct CachedExpr {
        exprtk::symbol_table<double> symbol_table;
        exprtk::expression<double> expression;
        double b[256];
        double w[128];
        double x; // [新增] 用于写入公式的输入变量

        CachedExpr() {
            for (int i = 0; i < 256; ++i) {
                b[i] = 0.0;
                symbol_table.add_variable("b" + std::to_string(i), b[i]);
                symbol_table.add_variable("B" + std::to_string(i), b[i]);
            }
            for (int i = 0; i < 128; ++i) {
                w[i] = 0.0;
                symbol_table.add_variable("w" + std::to_string(i), w[i]);
                symbol_table.add_variable("W" + std::to_string(i), w[i]);
            }
            // 注册变量 x 用于写入公式 (例如: x / 0.1)
            x = 0.0;
            symbol_table.add_variable("x", x);
            symbol_table.add_variable("X", x);

            expression.register_symbol_table(symbol_table);
        }
    };

    static ExpressionEvaluator& GetInstance() {
        static ExpressionEvaluator instance;
        return instance;
    }

    // Read公式计算: Raw Bytes -> Double
    bool Evaluate(const std::string& rule, const std::vector<uint8_t>& data, double& result) {
        if (rule.empty()) return false;
        auto entry = GetCachedExpr(rule);
        if (!entry) return false;

        for (size_t i = 0; i < data.size() && i < 256; ++i) {
            entry->b[i] = static_cast<double>(data[i]);
        }
        for (size_t i = 0; i + 1 < data.size() && i / 2 < 128; i += 2) {
            entry->w[i / 2] = static_cast<double>((data[i + 1] << 8) | data[i]);
        }

        result = entry->expression.value();
        return true;
    }

    // [新增] Write公式计算: Double -> Raw Bytes
    bool EvaluateWrite(const std::string& rule, double inputVal, int length, std::vector<uint8_t>& outBytes) {
        if (rule.empty()) return false;
        auto entry = GetCachedExpr(rule);
        if (!entry) return false;

        entry->x = inputVal;
        double rawVal = entry->expression.value();

        // 将计算结果(通常是整数)转换为字节数组
        // 默认使用小端序
        outBytes.clear();
        uint64_t valInt = static_cast<uint64_t>(rawVal); // 简单强转，可视需求加四舍五入

        for (int i = 0; i < length; ++i) {
            outBytes.push_back(static_cast<uint8_t>((valInt >> (i * 8)) & 0xFF));
        }
        return true;
    }

private:
    ExpressionEvaluator() = default;

    std::shared_ptr<CachedExpr> GetCachedExpr(const std::string& rule) {
        auto it = m_cache.find(rule);
        if (it == m_cache.end()) {
            auto new_expr = std::make_shared<CachedExpr>();
            exprtk::parser<double> parser;
            if (!parser.compile(rule, new_expr->expression)) {
                return nullptr;
            }
            m_cache[rule] = new_expr;
            return new_expr;
        }
        return it->second;
    }

    std::unordered_map<std::string, std::shared_ptr<CachedExpr>> m_cache;
};

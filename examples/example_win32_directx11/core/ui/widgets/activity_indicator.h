#pragma once

#include <chrono>

namespace I2CDebugger {

    class ActivityIndicator {
    public:
        ActivityIndicator() = default;

        // 触发指示灯
        void Trigger() {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTriggerTime).count();

            //灯亮过程中或灯灭50ms内，忽略触发
            if (m_isOn) {
                return;  // 灯亮时忽略
            }

            if (elapsed < 100) {  // 50ms亮 + 50ms灭 = 100ms周期内忽略
                return;
            }

            // 触发亮灯
            m_isOn = true;
            m_lastTriggerTime = now;
        }

        // 更新状态，返回当前是否亮灯
        bool Update() {
            if (!m_isOn) {
                return false;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTriggerTime).count();

            if (elapsed >= 50) {
                // 50ms后熄灭
                m_isOn = false;
            }

            return m_isOn;
        }
        bool IsOn() const { return m_isOn; }

    private:
        bool m_isOn = false;
        std::chrono::steady_clock::time_point m_lastTriggerTime;
    };

}

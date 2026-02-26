#pragma once
#include <memory>
#include <vector>
#include "core/models/channel_model.h"

namespace I2CDebugger {

    // ========== 【新增】波形图的配置数据结构 ==========
    struct PlotConfig {
        bool showPlotWindow = false;
        bool showPlotConfigWindow = false;
    };
    // =================================================

    class PlotViewModel {
    public:
        PlotViewModel() = default;
        // 【新增】获取配置数据的接口
        PlotConfig& GetConfig() { return m_config; }
        const PlotConfig& GetConfig() const { return m_config; }

        // ========== 【新增】时间轴管理 ==========
        void ResetTime() {
            m_startTime = static_cast<float>(ImGui::GetTime());
        }
        // ========== 【新增】运行与滚动状态管理 ==========
        void SetSystemRunning(bool running) {
            m_isSystemRunning = running;
            if (running) {
                m_isUserPaused = false; // 每次点击“开始触发”时，自动恢复滚动状态
            }
        }
        float GetRelativeTime() const {
            return static_cast<float>(ImGui::GetTime()) - m_startTime;
        }
        bool IsSystemRunning() const { return m_isSystemRunning; }

        void SetUserPaused(bool paused) { m_isUserPaused = paused; }
        bool IsUserPaused() const { return m_isUserPaused; }

        // 只有当系统在运行，且用户没有手动暂停时，画面才滚动
        bool IsScrolling() const { return m_isSystemRunning && !m_isUserPaused; }
        // ========================================
        // 添加新通道 (增加 id 参数)
        void AddChannel(size_t id, const std::string& name, ImVec4 color, int bufferSize = 2000) {
            m_channels.push_back(std::make_shared<ChannelModel>(id, name, color, bufferSize));
        }
        // 清空所有通道
        void ClearChannels() {
            m_channels.clear();
        }
        // 移除通道
        void RemoveChannel(size_t index) {
            if (index < m_channels.size()) {
                m_channels.erase(m_channels.begin() + index);
            }
        }

        // 接收解析后的单个数据点（根据 id 查找通道）
        void AddDataPoint(size_t entryIndex, float time, float value) {
            for (auto& ch : m_channels) {
                if (ch->id == entryIndex) {
                    ch->buffer.AddPoint(time, value);
                    break; // 找到对应通道后直接跳出
                }
            }
        }

        // 获取所有通道供 View 层绑定
        std::vector<std::shared_ptr<ChannelModel>>& GetChannels() {
            return m_channels;
        }

    private:
        std::vector<std::shared_ptr<ChannelModel>> m_channels;
        float m_startTime = 0.0f; // 记录开始时间
        // 【新增】保存状态的私有变量（默认停止运行）
        bool m_isSystemRunning = false;
        bool m_isUserPaused = false;
        // 【新增】配置实例
        PlotConfig m_config;
    };

} // namespace I2CDebugger

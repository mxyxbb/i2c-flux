#pragma once
#include <string>
#include <vector>
#include "imgui.h"

namespace I2CDebugger {

    // 高性能环形缓冲区，专为 ImPlot 设计
    struct ScrollingBuffer {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;

        ScrollingBuffer(int max_size = 2000) {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize) {
                Data.push_back(ImVec2(x, y));
            }
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }

        void Clear() {
            Data.shrink(0);
            Offset = 0;
        }
    };

    // 通道模型
    struct ChannelModel {
        size_t id;
        std::string name;
        bool isVisible = true;
        float scale = 1.0f;
        ImVec4 color;
        ScrollingBuffer buffer;

        ChannelModel(size_t channelId, const std::string& n, ImVec4 c, int max_size = 2000)
            : id(channelId), name(n), color(c), buffer(max_size) {
        }
    };

} // namespace I2CDebugger

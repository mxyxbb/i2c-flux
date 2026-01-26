#pragma once

namespace MyUi
{
    struct WindowManager
    {
        bool show_demo_window;
        bool show_framerate_window;
        bool show_i2c_simple_window;
    };
    void ShowMenuBar();
    void ShowFramerate();
    void DockspaceDemoBegin();
    void DockspaceDemoEnd();

    extern WindowManager g_WindowManager;
}

#pragma once
#include <windows.h>

class InputManager {
public:
    InputManager(HWND targetWindow);
    ~InputManager();

    // Returns true if the Hotkey was pressed this frame
    bool CheckHotkey();

private:
    HWND m_Hwnd;
    int m_HotkeyID = 1;
};
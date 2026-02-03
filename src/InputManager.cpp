#include "InputManager.h"

InputManager::InputManager(HWND targetWindow) : m_Hwnd(targetWindow) {
    // Register Alt + I
    RegisterHotKey(NULL, m_HotkeyID, MOD_ALT, 'I');
}

InputManager::~InputManager() {
    UnregisterHotKey(NULL, m_HotkeyID);
}
/*bool InputManager::CheckHotkey() {
    MSG msg;
    // We only want to peek specifically for the WM_HOTKEY message
    // This prevents us from "stealing" other messages (like clicks or closes) 
    // that GLFW needs to see.
    if (PeekMessage(&msg, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {
        if (msg.wParam == m_HotkeyID) {
            return true;
        }
    }
    return false;
}*/
bool InputManager::CheckHotkey() {
    return false;
    MSG msg;
    // Non-blocking peek for messages
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_HOTKEY && msg.wParam == m_HotkeyID) {
            return true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return false;
}
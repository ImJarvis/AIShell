#pragma once
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <windows.h>

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool ShouldClose();
    void PollEvents();
    void SwapBuffers();
    
    // Visibility Controls
    void Show();
    void Hide();
    bool IsVisible() const { return m_IsVisible; }
    bool ProcessOSMessages(int hotkeyId);

    // Getters for other systems
    GLFWwindow* GetNativeHandle() { return m_Window; }
    HWND GetWin32Handle();

private:
    GLFWwindow* m_Window;
    bool m_IsVisible = false;
};
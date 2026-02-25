#include "Window.h"

Window::Window(int width, int height, const char* title) {
    glfwInit();
    
    // Resizable & Opaque Setup
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Start hidden

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1); // Enable VSync
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Window::ShouldClose() { return glfwWindowShouldClose(m_Window); }
void Window::PollEvents() { glfwPollEvents(); }
void Window::SwapBuffers() { glfwSwapBuffers(m_Window); }

void Window::Show() {
    glfwShowWindow(m_Window);
    glfwFocusWindow(m_Window);
    m_IsVisible = true;
}

void Window::Hide() {
    glfwHideWindow(m_Window);
    m_IsVisible = false;
}

bool Window::ProcessOSMessages(int hotkeyId) {
    MSG msg;
    bool hotkeyPressed = false;

    // We process ALL messages here to ensure nothing gets stuck
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_HOTKEY && (int)msg.wParam == hotkeyId) {
            hotkeyPressed = true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return hotkeyPressed;
}

HWND Window::GetWin32Handle() {
    return glfwGetWin32Window(m_Window);
}

void Window::GetPosition(int* x, int* y) {
    glfwGetWindowPos(m_Window, x, y);
}

void Window::SetPosition(int x, int y) {
    glfwSetWindowPos(m_Window, x, y);
}
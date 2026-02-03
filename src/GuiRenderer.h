#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>

class GuiRenderer {
public:
    GuiRenderer(GLFWwindow* window);
    ~GuiRenderer();

    void BeginFrame();
    void EndFrame();
    
    // This is where we will add our UI code
    void RenderUI(int width, int height); 
};
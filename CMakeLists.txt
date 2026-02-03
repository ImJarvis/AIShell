cmake_minimum_required(VERSION 3.12)

# Updated Project Name
project(AIHollowShell)

set(CMAKE_CXX_STANDARD 17)

# 1. Setup ImGui Sources
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
file(GLOB IMGUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp 
    ${IMGUI_DIR}/imgui_draw.cpp 
    ${IMGUI_DIR}/imgui_tables.cpp 
    ${IMGUI_DIR}/imgui_widgets.cpp 
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 2. Setup Our Source Code (in src folder)
file(GLOB APP_SOURCES "src/*.cpp")
file(GLOB APP_HEADERS "src/*.h")

# 3. Setup GLFW (Local folder)
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/glfw") 

# 4. Create Executable (Updated Name: AIHollowShell)
add_executable(AIHollowShell main.cpp ${APP_SOURCES} ${APP_HEADERS} ${IMGUI_SOURCES})

# 5. Include Paths
target_include_directories(AIHollowShell PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends 
    ${GLFW_DIR}/include
)

# 6. Link Libraries
target_link_libraries(AIHollowShell PRIVATE "${GLFW_DIR}/lib-vc2022/glfw3.lib" opengl32)
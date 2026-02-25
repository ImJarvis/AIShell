cmake_minimum_required(VERSION 3.12)
project(AIHollowShell)

# ---- TinyLlama GGUF model setup ----
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/models")
set(TINYLLAMA_GGUF "${MODEL_DIR}/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf")

file(MAKE_DIRECTORY "${MODEL_DIR}")

if (NOT EXISTS "${TINYLLAMA_GGUF}")
    message(STATUS "Downloading TinyLlama GGUF model...")

    file(DOWNLOAD
        https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
        "${TINYLLAMA_GGUF}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_CODE)
    if (NOT DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download TinyLlama GGUF model")
    endif()
endif()


set(CMAKE_CXX_STANDARD 17)

# 1. Setup Directories
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/glfw")
set(LLAMA_DIR "${CMAKE_SOURCE_DIR}/llama.cpp")

# 2. Collect your App Sources
file(GLOB IMGUI_SOURCES 
    "${IMGUI_DIR}/imgui*.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
file(GLOB APP_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# 3. Create a STUB for the missing build info
# This breaks the cycle. We don't need llama.cpp to generate build.cpp anymore.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/build_info_stub.cpp" 
    "int LLAMA_BUILD_NUMBER = 0;\n"
    "char const *  LLAMA_COMMIT = \"unknown\";\n"
    "char const *  LLAMA_COMPILER = \"msvc\";\n"
    "char const *  LLAMA_BUILD_TARGET = \"x64\";\n"
)

# 4. Define the Llama Common files manually
# We include log.cpp to fix the logging errors.
set(COMMON_HELPER_SRCS
    "${LLAMA_DIR}/common/common.cpp"
    "${LLAMA_DIR}/common/sampling.cpp"
    "${LLAMA_DIR}/common/log.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/build_info_stub.cpp" # Use our fake file
)

# 5. Add llama.cpp sub-project (for the core libraries only)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) 
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
add_subdirectory(llama.cpp)

# 6. Create Executable
add_executable(AIHollowShell 
    main.cpp 
    ${APP_SOURCES} 
    ${IMGUI_SOURCES}
    ${COMMON_HELPER_SRCS}
    "src/CommandParser.cpp"
)

# 6b. Create Test Executable
add_executable(ShellTests 
    tests/TestRunner.cpp 
    "src/CommandParser.cpp"
)

# 7. Include Paths
target_include_directories(AIHollowShell PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends" 
    "${GLFW_DIR}/include"
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/common"
)

target_include_directories(ShellTests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

# 8. Linking
target_link_libraries(AIHollowShell PRIVATE 
    llama       
    ggml
    opengl32
    "${GLFW_DIR}/lib-vc2022/glfw3.lib"
)

target_link_libraries(ShellTests PRIVATE)

# 9. MSVC Fixes
if(MSVC)
    target_compile_definitions(AIHollowShell PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    #set_target_properties(AIHollowShell PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
endif()

# Automatically copy all runtime DLLs next to the exe
add_custom_command(TARGET AIHollowShell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:AIHollowShell>
        $<TARGET_FILE_DIR:AIHollowShell>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TINYLLAMA_GGUF}"
        $<TARGET_FILE_DIR:AIHollowShell>

    COMMAND_EXPAND_LISTS
)

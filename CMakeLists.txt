cmake_minimum_required(VERSION 3.12)
project(AIHollowShell)

# ---- GGUF Model Download Automation ----
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/models")
file(MAKE_DIRECTORY "${MODEL_DIR}")

# 2. Qwen Setup
set(QWEN_GGUF "${MODEL_DIR}/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf")
if (NOT EXISTS "${QWEN_GGUF}")
    message(STATUS "Downloading Qwen 2.5 Coder 1.5B GGUF...")
    file(DOWNLOAD https://huggingface.co/bartowski/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/Qwen2.5-Coder-1.5B-Instruct-Q4_K_M.gguf "${QWEN_GGUF}" SHOW_PROGRESS)
endif()

# 3. Llama 3.2 Setup
set(LLAMA3_GGUF "${MODEL_DIR}/Llama-3.2-3B-Instruct-Q4_K_M.gguf")
if (NOT EXISTS "${LLAMA3_GGUF}")
    message(STATUS "Downloading Meta-Llama 3.2 3B Instruct GGUF...")
    file(DOWNLOAD https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf "${LLAMA3_GGUF}" SHOW_PROGRESS)
endif()

# 4. Gemma 3 Setup
set(GEMMA_GGUF "${MODEL_DIR}/google_gemma-3-4b-it-q4_k_m.gguf")
# Robust check: if file is non-existent OR suspiciously small (<1MB), re-download
if (EXISTS "${GEMMA_GGUF}")
    file(SIZE "${GEMMA_GGUF}" GEMMA_SIZE)
    if (GEMMA_SIZE LESS 1000000)
        file(REMOVE "${GEMMA_GGUF}")
    endif()
endif()

if (NOT EXISTS "${GEMMA_GGUF}")
    message(STATUS "Downloading Gemma 3 4B Instruct GGUF (Bartowski)...")
    file(DOWNLOAD https://huggingface.co/bartowski/google_gemma-3-4b-it-GGUF/resolve/main/google_gemma-3-4b-it-Q4_K_M.gguf "${GEMMA_GGUF}" SHOW_PROGRESS)
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
)

# 6b. Create Test Executable
add_executable(ShellTests 
    tests/TestRunner.cpp 
    "src/CommandParser.cpp"
    "src/LlamaManager.cpp"
    "src/JsonKnowledgeBase.cpp"
    "src/WebSearchManager.cpp"
    ${COMMON_HELPER_SRCS}
)

# 7. Include Paths
target_include_directories(AIHollowShell PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends" 
    "${GLFW_DIR}/include"
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/ggml/include"
    "${LLAMA_DIR}/common"
)

target_include_directories(ShellTests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tests"
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/ggml/include"
    "${LLAMA_DIR}/common"
)

# 8. Linking
target_link_libraries(AIHollowShell PRIVATE 
    llama       
    ggml
    winhttp
    opengl32
    "${GLFW_DIR}/lib-vc2022/glfw3.lib"
)

target_link_libraries(ShellTests PRIVATE 
    llama 
    ggml
    winhttp
)

# 9. MSVC Fixes
if(MSVC)
    target_compile_definitions(AIHollowShell PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    #set_target_properties(AIHollowShell PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
endif()

# Automatically copy all runtime DLLs next to the exe
add_custom_command(TARGET AIHollowShell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:AIHollowShell>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:AIHollowShell>
        $<TARGET_FILE_DIR:AIHollowShell>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QWEN_GGUF}"
        $<TARGET_FILE_DIR:AIHollowShell>/models/

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LLAMA3_GGUF}"
        $<TARGET_FILE_DIR:AIHollowShell>/models/

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GEMMA_GGUF}"
        $<TARGET_FILE_DIR:AIHollowShell>/models/

    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET ShellTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ShellTests>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QWEN_GGUF}"
        $<TARGET_FILE_DIR:ShellTests>/models/
)

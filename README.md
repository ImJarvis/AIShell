# AI Hollow Shell 🐚

**AI Hollow Shell** is a specialized, local-first Windows CLI assistant designed for system administrators and power users. It leverages local LLMs (Large Language Models) to translate natural language into powerful terminal commands, providing a seamless bridge between human intent and machine execution.

![AI Shell Interface Mockup](https://via.placeholder.com/800x400?text=AI+Hollow+Shell+-+Premium+Windows+CLI+Experience)

## 🚀 Key Features

- **Local LLM Execution**: Powered by `llama.cpp`, supporting GGUF models like Qwen 2.5 and Phi-3.5 without needing cloud APIs.
- **Intelligent Shell Selection**: Automatically detects whether a command should run in **CMD** or **PowerShell** based on syntax (Cmdlets, operators like `&&`, etc.).
- **Specialized AI Persona**: A domain-locked assistant strictly focused on Windows administration, automation, and CLI tasks.
- **Robust Process Control**: Featuring a **Kill Switch** (STOP button) to safely terminate long-running or runaway commands instantly.
- **Premium UI/UX**: A state-of-the-art ImGui interface with:
  - **Centered "Heartbeat" Status**: Real-time visibility of system states (Thinking, Loading, Ready).
  - **Asynchronous Workflow**: Non-blocking model loading and command execution.
  - **Safety First**: Visual verification indicators for suggested commands.
- **Anti-Hallucination Guardrails**: Specialized parsing logic that strips internal AI noise and strictly enforces flat JSON schemas.

## 🛠️ Technology Stack

- **Core**: C++ 17/20
- **Graphics & UI**: ImGui, GLFW, OpenGL
- **AI Inference**: llama.cpp (Local GGUF loading)
- **OS Integration**: Win32 API for process management and pipe redirection

## 📂 Project Structure

- `src/`: Core application logic, window management, and UI rendering.
- `llama.cpp/`: Submodule for model inference.
- `tests/`: Module-based test suite for parsing and shell execution.
- `models/`: (Recommended) Directory for storing your `.gguf` model files.

## 🏁 Getting Started

### Prerequisites
- Visual Studio 2022 (with C++ development workload)
- CMake
- Git

### Installation
1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/YourUsername/AIHollowShell.git
    cd AIHollowShell
    git submodule update --init --recursive
    ```
2.  **Build the Project**:
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```
3.  **Add Models**:
    Download a compatible GGUF model (e.g., `qwen2.5-coder-1.5b-instruct-q4_k_m.gguf`) and place it in the identified model directory.

## 📖 Usage

1.  **Launch**: Run `AIHollowShell.exe`.
2.  **Select Model**: Use the dropdown in the header to switch between available LLMs.
3.  **Type Intent**: Enter a natural language request like *"Show my IP and list active network results"*.
4.  **Execute**: Review the generated command and click **RUN COMMAND** or press Enter.
5.  **Control**: Use the blinking **STOP !!** button in the terminal header to kill a task if it takes too long.

## 🛡️ License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
*Built with passion for the Windows terminal ecosystem.*

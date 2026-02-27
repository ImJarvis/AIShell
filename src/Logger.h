#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

class Logger {
public:
    enum Level { DEBUG, INFO, WARNING, ERR };

    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void Log(Level level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        tm = *std::localtime(&time);
#endif

        std::string levelStr;
        switch (level) {
            case DEBUG: levelStr = "[DEBUG]"; break;
            case INFO:  levelStr = "[INFO]"; break;
            case WARNING: levelStr = "[WARN]"; break;
            case ERR:   levelStr = "[ERROR]"; break;
        }

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " " << levelStr << " " << message << std::endl;
        
        if (m_File.is_open()) {
            m_File << ss.str();
            m_File.flush();
        }

        // Silent in console unless it's an Error
        if (level == ERR) {
            std::cerr << ss.str();
        }
    }

private:
    Logger() {
        std::filesystem::create_directory("logs");
        m_File.open("logs/app.log", std::ios::app);
    }
    ~Logger() {
        if (m_File.is_open()) m_File.close();
    }

    std::ofstream m_File;
    std::mutex m_Mutex;
};

#define LOG_DEBUG(msg) Logger::Get().Log(Logger::DEBUG, msg)
#define LOG_INFO(msg)  Logger::Get().Log(Logger::INFO, msg)
#define LOG_WARN(msg)  Logger::Get().Log(Logger::WARNING, msg)
#define LOG_ERROR(msg) Logger::Get().Log(Logger::ERR, msg)

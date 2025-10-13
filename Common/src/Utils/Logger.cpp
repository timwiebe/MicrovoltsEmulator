#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <format>
#include <memory>
#include "../../include/Utils/Logger.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Utils
{
    void Logger::enableAnsiEscapeCodes()
    {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return;
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }

    void Logger::printToConsole(const std::string& message, LogType type)
    {
        std::call_once(initFlag, enableAnsiEscapeCodes);

        if (m_loggingEnabled)
        {
            switch (type)
            {
            case LogType::Info:
                std::cout << LogColors::Info << "[Info] " << message << LogColors::Reset << "\n";
                break;
            case LogType::Error:
                std::cout << LogColors::Error << "[Error] " << message << LogColors::Reset << "\n";
                break;
            case LogType::Normal:
                std::cout << LogColors::Normal << message << LogColors::Reset << "\n";
                break;
            case LogType::Warning:
                std::cout << LogColors::Warning << "[Warning] " << message << LogColors::Reset << "\n";
                break;
            }
        }
    }

    void Logger::log(const std::string& message, LogType type, const std::string& functionName)
    {
        std::string logMessage = message;
        if (!functionName.empty())
        {
            logMessage = "[" + functionName + "] " + logMessage;
        }
        printToConsole(logMessage, type);
        newline();
    }

    std::string Logger::getCurrentDateTime()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        std::tm tm_time{};
#ifdef _WIN32
        localtime_s(&tm_time, &now_c);   // Windows
#else
        localtime_r(&now_c, &tm_time);   // Linux/Unix
#endif

        std::stringstream ss;
        ss << std::put_time(&tm_time, "%Y-%m-%d %X");
        return ss.str();
    }

    std::string Logger::logTypeToString(LogType type)
    {
        switch (type) {
        case LogType::Info: return "Info";
        case LogType::Error: return "Error";
        case LogType::Normal: return "Normal";
        case LogType::Warning: return "Warning";
        }
        return "Unknown";
    }
}


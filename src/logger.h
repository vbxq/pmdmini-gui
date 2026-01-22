#pragma once

#include <string>

class Logger {
public:
    static void Init();
    static void Shutdown();

    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);
};

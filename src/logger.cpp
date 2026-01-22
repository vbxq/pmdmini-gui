#include "logger.h"
#include <iostream>

void Logger::Init()
{
    // nothing to init for now
}

void Logger::Shutdown()
{
    // nothing to cleanup
}

static void Log(const char *level, const std::string &msg)
{
    std::cerr << "[" << level << "] " << msg << "\n";
}

void Logger::Info(const std::string &msg)
{
    Log("info", msg);
}

void Logger::Warn(const std::string &msg)
{
    Log("warn", msg);
}

void Logger::Error(const std::string &msg)
{
    Log("error", msg);
}

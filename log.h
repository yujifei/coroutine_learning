#pragma once

#include <string>
#include <cstdio>
#include <mutex>

class Logger
{
public:
    static Logger& instance();
    void writeLog(const char* fmt, ...);
    ~Logger();
private:
    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;
    void init();
private:
    std::mutex m_lock;
    FILE* m_fp = nullptr;
};
#define info(fmt, ...) Logger::instance().writeLog(fmt##"\n", __VA_ARGS__)
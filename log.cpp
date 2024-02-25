#include "log.h"
#include <chrono>
#include <cstdarg>

Logger& Logger::instance() 
{
    static Logger s_logger;
    return s_logger;
}

Logger::Logger()
{
    init();
}

Logger::~Logger()
{
    fclose(m_fp);
    m_fp = nullptr;
}

void Logger::writeLog(const char* fmt, ...)
{
    std::lock_guard<std::mutex> lg(m_lock);
    std::va_list args;
    va_start(args, fmt);
    vfprintf(m_fp, fmt, args);
    va_end(args);
    fflush(m_fp);
}

void Logger::init()
{
    std::string log_file("log_");
    log_file += std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    log_file += ".txt";
    m_fp = fopen(log_file.c_str(), "w+");
}
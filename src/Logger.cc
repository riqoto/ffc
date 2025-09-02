#include "Logger.hh"
#include <iostream>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

Logger::Logger(const std::string& filename) {
    log_file_.open(filename, std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "HATA: Log dosyasi acilamadi: " << filename << std::endl;
    }
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    
    // Windows ve diğer platformlar için güvenli zaman dönüşümü
    #ifdef _WIN32
        localtime_s(&buf, &in_time_t);
    #else
        localtime_r(&in_time_t, &buf);
    #endif

    auto time_str_stream = std::stringstream();
    time_str_stream << std::put_time(&buf, "%Y-%m-%d %X");

    std::cout << "[" << time_str_stream.str() << "] " << message << std::endl;
    if (log_file_.is_open()) {
        log_file_ << "[" << time_str_stream.str() << "] " << message << std::endl;
    }
}
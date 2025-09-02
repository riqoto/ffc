#pragma once

#include <string>
#include <fstream>
#include <mutex>

// Thread-safe loglama sınıfı. Hem dosyaya hem konsola yazar.
class Logger {
public:
    // Logger'ı belirtilen log dosyası ile başlatır.
    Logger(const std::string& filename);

    // Senkronize bir şekilde log mesajı yazar.
    void log(const std::string& message);

private:
    std::mutex mtx_;
    std::ofstream log_file_;
};
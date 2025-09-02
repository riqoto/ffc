#pragma once

#include <filesystem>
#include <vector>
#include <atomic>
#include <string>
#include <unordered_set>
#include "Logger.hh"
#include "TSQueue.hh"

namespace fs = std::filesystem;

struct CopyJob {
    fs::path source_path;
    fs::path dest_path;
    uintmax_t file_size;
};

class DirectoryCopier {
public:
    DirectoryCopier(fs::path source, fs::path dest);
    void run();

private:
    // Adım 1.1: Dosya keşif işlemini ayrı bir thread'de çalıştırır.
    void discover_files_task();
    // Adım 1.2: Keşif sırasında konsolda spinner ve sayaç gösterir.
    void discovery_progress_reporter_task(const std::atomic<bool>& is_done);

    // Adım 2: Kopyalama öncesi hedef dizindeki dosyaları hafızaya alır.
    void populate_destination_files();

    // Adım 3: Worker ve ilerleme thread'lerini başlatır.
    void execute_copy();

    // Worker thread'lerin çalıştıracağı fonksiyon (artık set parametresi alıyor).
    void worker_function(const std::unordered_set<std::string>& dest_files);
    
    // Anlık kopyalama ilerlemesini konsola yazan fonksiyon.
    void copy_progress_reporter_function();

    fs::path source_dir_;
    fs::path dest_dir_;
    Logger logger_;

    std::vector<CopyJob> all_jobs_;
    uintmax_t total_size_ = 0;
    std::atomic<size_t> files_discovered_{0};

    // Performans için hedef dosya isimlerinin tutulduğu hash set.
    std::unordered_set<std::string> dest_files_cache_;

    TSQueue<CopyJob> job_queue_;
    std::atomic<uintmax_t> total_bytes_copied_{0};
    std::atomic<size_t> files_processed_{0};
    std::atomic<bool> copy_finished_{false};
};
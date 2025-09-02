#include "DirectoryCopier.hh"
#include "Utils.hh"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>

DirectoryCopier::DirectoryCopier(fs::path source, fs::path dest)
    : source_dir_(std::move(source)), 
      dest_dir_(std::move(dest)), 
      logger_("file_copier.log") {}

void DirectoryCopier::run() {
    // --- Girdi Doğrulama ---
    if (!fs::exists(source_dir_) || !fs::is_directory(source_dir_)) {
        logger_.log("HATA: Kaynak dizin bulunamadi veya bir dizin degil: " + source_dir_.string());
        return;
    }
    if (!fs::exists(dest_dir_)) {
        logger_.log("INFO: Hedef dizin mevcut degil, olusturuluyor: " + dest_dir_.string());
        fs::create_directories(dest_dir_);
    } else if (!fs::is_directory(dest_dir_)) {
        logger_.log("HATA: Hedef yol mevcut ancak bir dizin degil: " + dest_dir_.string());
        return;
    }

    // --- Faz 1: Keşif (UX İyileştirmesi ile) ---
    logger_.log("Faz 1: Dosyalar kesfediliyor...");
    std::atomic<bool> discovery_done = false;
    std::thread discovery_thread(&DirectoryCopier::discover_files_task, this);
    std::thread progress_thread(&DirectoryCopier::discovery_progress_reporter_task, this, std::ref(discovery_done));
    
    discovery_thread.join();
    discovery_done = true;
    progress_thread.join();

    logger_.log("Kesif tamamlandi. Toplam " + std::to_string(all_jobs_.size()) + " adet dosya bulundu (" + Utils::format_bytes(total_size_) + ").");
    if (all_jobs_.empty()) {
        logger_.log("Kopyalanacak dosya bulunamadi.");
        return;
    }

    // --- Faz 2: Hedef Dizin Analizi (Performans Optimizasyonu) ---
    logger_.log("Faz 2: Hedef dizin analizi yapiliyor...");
    populate_destination_files();
    logger_.log("Hedef dizin analizi tamamlandi. " + std::to_string(dest_files_cache_.size()) + " adet mevcut dosya bulundu.");

    // --- Faz 3: Yürütme ---
    execute_copy();
    
    logger_.log("Tum islemler tamamlandi.");
}

void DirectoryCopier::discover_files_task() {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(source_dir_, fs::directory_options::skip_permission_denied)) {
            // Hata kontrolü: Bazı dosyalar okunamayabilir.
            std::error_code ec;
            if (entry.is_regular_file(ec) && !ec) {
                uintmax_t size = fs::file_size(entry.path(), ec);
                if (!ec) {
                    all_jobs_.push_back({entry.path(), dest_dir_ / entry.path().filename(), size});
                    total_size_ += size;
                    files_discovered_++;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Bu thread'den gelen hataları ana log'a yazdıralım.
        // Normalde daha gelişmiş bir thread-safe hata mekanizması gerekir.
        logger_.log("HATA (Discovery): " + std::string(e.what()));
    }
}

void DirectoryCopier::discovery_progress_reporter_task(const std::atomic<bool>& is_done) {
    const char* spinner = "|/-\\";
    int spinner_idx = 0;
    while (!is_done) {
        std::cout << "\r" << "Kesfediliyor... [" << spinner[spinner_idx++ % 4] << "] "
                  << files_discovered_.load() << " dosya bulundu." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Temiz bir bitiş için son durumu yaz.
    std::cout << "\r" << "Kesfediliyor... [OK] " << files_discovered_.load() << " dosya bulundu." << std::string(10, ' ') << std::endl;
}

void DirectoryCopier::populate_destination_files() {
    try {
        for (const auto& entry : fs::directory_iterator(dest_dir_)) {
            if (entry.is_regular_file()) {
                dest_files_cache_.insert(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        logger_.log("UYARI: Hedef dizin okunurken hata olustu: " + std::string(e.what()));
    }
}

void DirectoryCopier::execute_copy() {
    logger_.log("Faz 3: Kopyalama islemi baslatiliyor...");
    
    const unsigned int num_threads = std::min(4u, std::thread::hardware_concurrency());
    logger_.log(std::to_string(num_threads) + " adet islemci is parcacigi kullanilacak.");

    std::vector<std::thread> worker_threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        // Artık worker'lara önbelleği (cache) de gönderiyoruz.
        worker_threads.emplace_back(&DirectoryCopier::worker_function, this, std::cref(dest_files_cache_));
    }
    
    std::thread progress_thread(&DirectoryCopier::copy_progress_reporter_function, this);

    for (const auto& job : all_jobs_) {
        job_queue_.push(job);
    }
    job_queue_.notify_finished();

    for (auto& t : worker_threads) {
        t.join();
    }
    
    copy_finished_ = true;
    progress_thread.join();
}

void DirectoryCopier::worker_function(const std::unordered_set<std::string>& dest_files) {
    while (true) {
        CopyJob job;
        if (!job_queue_.pop(job)) {
            break;
        }

        try {
            // OPTİMİZASYON: Disk yerine hafızadaki set'e soruyoruz.
            if (dest_files.count(job.dest_path.filename().string())) {
                logger_.log(job.source_path.filename().string() + " - STATUS: Zaten var, atlandi.");
            } else {
                auto start_time = std::chrono::high_resolution_clock::now();
                fs::copy_file(job.source_path, job.dest_path, fs::copy_options::none);
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                
                logger_.log(job.source_path.filename().string() + " - STATUS: Kopyalandi (" + std::to_string(duration) + " ms)");
                total_bytes_copied_ += job.file_size;
            }
        } catch (const fs::filesystem_error& e) {
            logger_.log("HATA: " + job.source_path.string() + " kopyalanamadi. Sebep: " + e.what());
        }
        files_processed_++;
    }
}

void DirectoryCopier::copy_progress_reporter_function() {
    auto start_time = std::chrono::steady_clock::now();
    while (!copy_finished_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        size_t processed = files_processed_.load();
        uintmax_t copied = total_bytes_copied_.load();
        
        if (processed == 0 || total_size_ == 0) continue;

        auto now = std::chrono::steady_clock::now();
        double elapsed_seconds = std::chrono::duration<double>(now - start_time).count();
        double speed_mbps = (elapsed_seconds > 0) ? (copied / (1024.0 * 1024.0)) / elapsed_seconds : 0.0;
        double progress_percent = (static_cast<double>(copied) / total_size_) * 100.0;
        
        uintmax_t remaining_bytes = total_size_ > copied ? total_size_ - copied : 0;
        double eta_seconds = (speed_mbps > 0.01) ? (remaining_bytes / (1024.0 * 1024.0)) / speed_mbps : 0.0;
        
        std::cout << "\r" << std::fixed << std::setprecision(2)
                  << "Ilerleme: %" << progress_percent 
                  << " [" << processed << "/" << all_jobs_.size() << " dosya]"
                  << " [" << Utils::format_bytes(copied) << "/" << Utils::format_bytes(total_size_) << "]"
                  << " Hiz: " << speed_mbps << " MB/s"
                  << " ETA: " << static_cast<int>(eta_seconds / 60) << "dk " << static_cast<int>(eta_seconds) % 60 << "sn"
                  << std::string(10, ' ');
        std::cout.flush();
    }
    std::cout << "\r" << "Ilerleme: %100.00" 
              << " [" << all_jobs_.size() << "/" << all_jobs_.size() << " dosya]"
              << " [" << Utils::format_bytes(total_size_) << "/" << Utils::format_bytes(total_size_) << "]"
              << " Tamamlandi." << std::string(20, ' ') << std::endl;
}
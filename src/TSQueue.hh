#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class TSQueue {
public:
    // Kuyruğa yeni bir eleman ekler ve bekleyen bir thread'i uyandırır.
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    // Kuyruktan bir eleman çeker. Kuyruk boşsa ve işlem bitmediyse bekler.
    // Eğer işlem bittiyse ve kuyruk boşsa false döner.
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty() || finished_; });
        if (queue_.empty() && finished_) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Kuyruğa daha fazla eleman eklenmeyeceğini bildirir ve bekleyen tüm thread'leri uyandırır.
    void notify_finished() {
        std::lock_guard<std::mutex> lock(mtx_);
        finished_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool finished_ = false;
};
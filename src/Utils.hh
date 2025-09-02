#pragma once
#include <string>
#include <cstdint> // uintmax_t için

namespace Utils {
    // Bayt cinsinden boyutu okunabilir bir formata (KB, MB, GB) çevirir.
    std::string format_bytes(uintmax_t bytes);
}
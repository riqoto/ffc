#include "Utils.hh"
#include <sstream>
#include <iomanip>

namespace Utils {
    std::string format_bytes(uintmax_t bytes) {
        if (bytes < 1024) return std::to_string(bytes) + " B";
        double kb = bytes / 1024.0;
        if (kb < 1024.0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << kb;
            return ss.str() + " KB";
        }
        double mb = kb / 1024.0;
        if (mb < 1024.0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << mb;
            return ss.str() + " MB";
        }
        double gb = mb / 1024.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << gb;
        return ss.str() + " GB";
    }
}
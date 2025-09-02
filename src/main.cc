#include "DirectoryCopier.hh"
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    // Windows'ta konsolun UTF-8 karakterleri doğru göstermesi için
    SetConsoleOutputCP(CP_UTF8);
    #endif

    if (argc != 3) {
        std::cerr << "Kullanim: " << argv[0] << " <kaynak_dizin> <hedef_dizin>" << std::endl;
        return 1;
    }

    try {
        fs::path source(argv[1]);
        fs::path dest(argv[2]);

        DirectoryCopier copier(source, dest);
        copier.run();

    } catch (const std::exception& e) {
        std::cerr << "Beklenmedik bir hata olustu: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#include <string>
#include <iostream>
#include <cstring> // For std::memset()
#include <cstdlib> // For std::mbstowcs()
#include "nameshape.h"

constexpr int argv_max = 256;

#ifndef WIN32
int main(int argc, char* argv[]) {
    wchar_t** args = new wchar_t*[argc];

    for (int i = 0; i < argc; ++i) {
        args[i] = new wchar_t[argv_max];

        std::memset(args[i], 0, argv_max);
        std::mbstowcs(args[i], argv[i], argv_max);
    }
#else
int wmain(int argc, wchar_t* argv[]) {
    auto args = argv;
#endif

    Nameshape nameshape;

    try {
        if (nameshape.parseCommandLine(argc, args)) {
            nameshape.performNameshape();
        }
    } catch (std::exception& e) {
        std::wcout << e.what() << std::endl;
    }

    return 0;
}


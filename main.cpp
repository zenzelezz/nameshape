#include <string>
#include <iostream>
#include "nameshape.h"

int wmain(int argc, wchar_t* argv[]) {
	Nameshape nameshape;

	try {
		if (nameshape.parseCommandLine(argc, argv)) {
			nameshape.performNameshape();
		}
	} catch (std::exception& e) {
		std::wcout << e.what() << std::endl;
	}

	return 0;
}

#pragma once

#include <string>
#include <regex>
#include <filesystem>

/**
 * TODO: Implement recursive renaming
 */

class Nameshape {
	bool sort;
	bool confirm;
	bool verbose;
	std::filesystem::path directory;
	std::wregex input;
	std::wstring output;

public:
	Nameshape();
	~Nameshape();

	bool parseCommandLine(int argc, wchar_t* argv[]);
	void performNameshape();
};

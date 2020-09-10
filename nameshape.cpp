#include "nameshape.h"

#include <regex>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm> // For std::sort()
#include <cwchar> // For std::swprintf()
#include <boost/program_options.hpp>

namespace po = boost::program_options;

Nameshape::Nameshape() {
	this->sort      = false;
	this->confirm   = false;
	this->verbose   = true;
	this->input     = std::wregex(L"(.*)");
	this->directory = std::filesystem::current_path();
}

Nameshape::~Nameshape() {
	//
}

bool Nameshape::parseCommandLine(int argc, wchar_t* argv[]) {
	po::options_description description("Available options");

	description.add_options()
		("sort",                                  "Sort files before renaming")
		("confirm",                               "Prompt for confirmation before each rename operation")
		("quiet",                                 "Do not print progress messages")
		("directory", po::wvalue<std::wstring>(), "The directory in which to find and rename files. (Default = current directory)")
		("input",     po::wvalue<std::wstring>(), "C++ regular expression (extended ECMAScript)")
		("output",    po::wvalue<std::wstring>(), "Output format string. See below for details.")
	;

	if (argc < 2) {
		std::cout << description << std::endl;
		std::cout << "For the most part the output file name syntax is very basic." << std::endl;
		std::cout << "It is a plain string where the few following replacements can be made:" << std::endl;
		std::cout << "  %(name)       Stem of the original file name (\"test.txt\" -> \"test\")" << std::endl;
		std::cout << "  %(ext)        Extension of the original file name (\"test.txt\" -> \"txt\")" << std::endl;
		std::cout << "  %(counter)    Running counter, plain" << std::endl;
		std::cout << "  %(counter,N)  Running counter, %0Nd format" << std::endl;

		return false;
	}

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, description), vm);
	po::notify(vm);

	this->sort    = vm.count("sort");
	this->confirm = vm.count("confirm");
	this->verbose = !vm.count("queit");

	if (vm.count("directory")) {
		this->directory = vm["directory"].as<std::wstring>();

		if (!std::filesystem::is_directory(this->directory)) {
			throw std::runtime_error("Output path is not a directory");
		}
	}

	if (vm.count("input")) {
		try {
			this->input = std::wregex(vm["input"].as<std::wstring>());
		} catch (std::regex_error& e) {
			std::string message = std::string("Invalid regular expression for input: ") + e.what();
			throw std::runtime_error(message.c_str());
		}
	}

	if (vm.count("output")) {
		this->output = vm["output"].as<std::wstring>();
	}

	if (this->output.size() == 0) {
		throw std::runtime_error("Missing parameter: --output");
	}

	return true;
}

void Nameshape::performNameshape() {
	std::vector<std::wstring> fileNames;
	std::wsmatch inputResult;

	// Discover all files in the given directory
	for (auto& file : std::filesystem::directory_iterator(this->directory)) {
		if (!file.is_regular_file()) {
			continue;
		}

		std::wstring fileName = file.path().filename().wstring();

		if (std::regex_match(fileName, inputResult, this->input)) {
			fileNames.push_back(fileName);
		}
	}

	// Sort the files, if it requested
	if (this->sort) {
		std::sort(fileNames.begin(), fileNames.end());
	}

	// Iterate and rename the files
	const std::wregex  counterRegex(L"%\\(counter(,[0-9])?\\)");
	const std::wstring nameMatch(L"%(name)");
	const std::wstring extMatch(L"%(ext)");

	std::wsmatch counterResult;
	constexpr int maxCounterLength = 10;

	std::string::size_type matchPosition = std::string::npos;

	for (std::vector<std::wstring>::size_type i = 0; i < fileNames.size(); ++i) {
		std::wstring outName = this->output;
		std::wstring inName  = fileNames[i];

		// Construct output file name
		std::filesystem::path inPath = this->directory / std::filesystem::path(inName);

		// Replace %(name)
		matchPosition = std::string::npos;

		std::wstring name = inPath.filename().stem().wstring();

		while ((matchPosition = outName.find(nameMatch)) != std::string::npos) {
			outName.replace(matchPosition, nameMatch.length(), name);
		}

		// Replace %(ext)
		matchPosition = std::string::npos;

		std::wstring extension = inPath.filename().extension().wstring().substr(1);

		while ((matchPosition = outName.find(extMatch)) != std::string::npos) {
			outName.replace(matchPosition, extMatch.length(), extension);
		}

		// Replace %(counter)
		while (std::regex_search(outName, counterResult, counterRegex)) {
			wchar_t number[maxCounterLength] = { 0 };
			auto numberMatch = counterResult[1];
			std::wstring format(L"%");

			if (numberMatch.matched) {
				format += numberMatch.str().replace(0, 1, L"0");
			}

			format += L"d";

			std::swprintf(number, maxCounterLength, format.c_str(), i);

			outName.replace(counterResult.position(0), counterResult.length(), number);
		}

		// Output file name should now be ready
		std::filesystem::path outPath = this->directory / outName;

		// Check whether the file already exists
		while (std::filesystem::exists(outPath)) {
			wchar_t action;

			std::wcout << L"File \"" << outName << L"\" already exists." << std::endl;
			std::wcout << L"Action: [O]verwrite, [R]ename, [I]gnore, [S]top >> ";
			std::wcin >> action;

			if (action == L'I' || action == L'i') {
				// [I]gnore - move to next file
				continue;
			} else  if (action == L'S' || action == L's') {
				// [S]top - don't go any further
				std::wcout << "Stopping processing." << std::endl;
				return;
			} else if (action == L'R' || action == L'r') {
				// [R]ename - prompt for a new name
				std::wcout << L"Enter new name (without path): ";
				std::wcin >> outName;
				outPath = this->directory / outName;
			} else if (action == L'O' || action == L'o') {
				// [O]verwrite - remove existing file
				std::filesystem::remove(outPath);
			} else {
				// Bad input
				std::wcout << L"Invalid action \"" << action << "\"; stopping processing." << std::endl;
				return;
			}
		}

		// Ask for confirmation if it was requested
		if (this->confirm) {
			wchar_t action = L'x';

			while (action != 'Y' && action != 'y' && action != 'N' && action != 'n') {
				std::wcout << L"Rename \"" << inName << L"\" to \"" << outName << L"\"?" << std::endl;
				std::wcout << L"Action: [Y]es, [N]o >> ";
				std::wcin >> action;
			}

			if (action == 'N' || action == 'n') {
				// Don't rename this file; move on to next
				continue;
			}
		}

		if (this->verbose) {
			std::wcout << L"Renaming \"" << inName << L"\" to \"" << outName << L"\"" << std::endl;
		}

		std::filesystem::rename(inPath, outPath);
	}
}

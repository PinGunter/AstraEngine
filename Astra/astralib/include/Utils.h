#pragma once
#include <vector>
#include <string>

namespace Astra {

	enum LOG_LEVELS{INFO, WARNING, ERR};

	std::vector<char> readFile(const std::string& filename);

	void Log(const std::string& s, LOG_LEVELS level);
}
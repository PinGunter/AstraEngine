#include <Utils.h>
#include <fstream>
#include <iostream>

std::vector<char> Astra::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary); // start reading at the end so we know file size
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file! (" + filename + ")");
	}
	// allocate for correct file size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0); // back to the begginning to read bytes
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

void Astra::Log(const std::string& s, LOG_LEVELS level)
{
	if (level == LOG_LEVELS::INFO) {
		std::cout << "[INFO] ";
	}
	else if (level == LOG_LEVELS::WARNING) {
		std::cout << "[WARNING] ";
	}
	std::cout << s << std::endl;
}

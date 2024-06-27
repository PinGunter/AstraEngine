#include <Utils.h>
#include <fstream>
#include <iostream>

std::vector<char> Astra::readFile(const std::string& filename, bool binary)
{
	std::ifstream file(filename, std::ios::ate | (binary ? std::ios::binary : 0)); // start reading at the end so we know file size
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
		std::cerr << "[INFO] ";
	}
	else if (level == LOG_LEVELS::WARNING) {
		std::cerr << "[WARNING] ";
	}
	else if (level == LOG_LEVELS::ERR) {
		std::cerr << "[ERROR] ";
	}
	std::cerr << s << std::endl;
}

void Astra::Log(const std::string& name, const glm::vec3& s, LOG_LEVELS level)
{
	auto x = std::to_string(s.x);
	auto y = std::to_string(s.y);
	auto z = std::to_string(s.z);

	Log(name + ": " + x + ", " + y + ", " + z, level);
}

void Astra::Log(const std::string& name, const glm::vec4& s, LOG_LEVELS level)
{
	auto x = std::to_string(s.x);
	auto y = std::to_string(s.y);
	auto z = std::to_string(s.z);
	auto w = std::to_string(s.w);

	Log(name + ": " + x + ", " + y + ", " + z + ", " + w, level);
}

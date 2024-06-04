#include <Utils.h>
#include <fstream>

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

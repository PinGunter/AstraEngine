#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
namespace Astra
{

	enum LOG_LEVELS
	{
		INFO,
		WARNING,
		ERR
	};

	std::vector<char> readFile(const std::string &filename, bool binary);

	void Log(const std::string &s, LOG_LEVELS level = INFO);

	void Log(const std::string &name, const glm::vec3 &s, LOG_LEVELS level = INFO);
	void Log(const std::string &name, const glm::vec4 &s, LOG_LEVELS level = INFO);

}
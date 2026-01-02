#pragma once
#include <deque>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

struct DeletionQueue
{
	std::deque<std::pair<std::function<void()>, std::string>> deletors;

	void push_function(std::function<void()>&& function, const std::string & name = "");

	void flush();
};
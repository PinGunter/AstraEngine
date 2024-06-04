#pragma once
#include <memory>

template<typename T>
class Singleton {
protected:
	static T* _instance;
	Singleton() {};
	~Singleton() {};

public:
	Singleton(const Singleton& s) = delete;
	Singleton& operator=(const Singleton) = delete;
	static T* getInstance();
};

template<typename T>
T* Singleton<T>::_instance;

template<typename T>
T* Singleton<T>::getInstance(){
	if (!_instance) {
		_instance = new T();
	}
	return _instance;
}
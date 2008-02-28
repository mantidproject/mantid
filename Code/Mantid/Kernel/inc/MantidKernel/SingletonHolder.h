#ifndef SINGLETON_HOLDER_H
#define SINGLETON_HOLDER_H

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Simplified from the original code, to just work for simple singletons
// Removed all the code relating to the creation/destruction, threading and 
// lifetime policies.
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <typeinfo>
#include <iostream>

namespace Mantid
{
namespace Kernel
{

/// prototype for function passed to atexit()
typedef void (*atexit_func_t)(void);
extern void CleanupSingletons(void);
extern void AddSingleton(atexit_func_t func);

template <typename T>
class SingletonHolder
{
public:
	static T& Instance();

private:
	static void DestroySingleton();
	SingletonHolder();

	static T* pInstance;
	static bool destroyed;
};

template <typename T> 
struct CreateUsingNew
{
	static T* Create(){return new T;}

	static void Destroy(T* p){delete p;}
};

template <typename T>
inline T& SingletonHolder<T>::Instance()
{
	if (destroyed)
	{
	    std::string s("Attempt to use destroyed singleton ");
	    s += typeid(T).name();
	    throw std::runtime_error(s.c_str());
	}
	if (!pInstance)
	{
//		std::cerr << "creating singleton " << typeid(T).name() << std::endl;
		pInstance = CreateUsingNew<T>::Create();
		AddSingleton(&DestroySingleton);
		atexit(&CleanupSingletons);
	}
	return *pInstance;
}

template <typename T>
void SingletonHolder<T>::DestroySingleton()
{
	assert(!destroyed);
//	std::cerr << "destroying singleton " << typeid(T).name() << std::endl;
	CreateUsingNew<T>::Destroy(pInstance);
	pInstance = 0;
	destroyed = true;
};

template <typename T>
T* SingletonHolder<T>::pInstance = 0;

template <typename T>
bool SingletonHolder<T>::destroyed = false;

}
}

#endif //SINGLETON_HOLDER_H


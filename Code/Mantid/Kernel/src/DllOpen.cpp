//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <iostream>

/*
     If the OS is Windows then LoadLibrary, GetProcAddress and FreeLibrary are used. 
     Some casting to HINSTANCE is required.
     Shared library name is of the form *.dll. 
     
     If the OS is Linux then dlopen, dlsym and dlclose are used.
     Shared library name is of the form lib*.so.
     
*/

#if _WIN32

#include "windows.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid
{
namespace Kernel
{

// Get a reference to the logger
Logger& DllOpen::log = Logger::get("DllOpen");

void* DllOpen::OpenDll(const std::string& libName)
{
	std::string str = libName + ".dll";
	return LoadLibrary(str.c_str());
}

void* DllOpen::GetFunction(void* lib, const std::string& funcName)
{
	return GetProcAddress((HINSTANCE)lib, funcName.c_str());
}

void DllOpen::CloseDll(void* lib)
{
	FreeLibrary((HINSTANCE)lib);
}

#else

#include "/usr/include/dlfcn.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid
{
namespace Kernel
{
	
// Get a reference to the logger
Logger& DllOpen::log = Logger::get("DllOpen");

void* DllOpen::OpenDll(const std::string& libName)
{
	std::string str = "lib" + libName + ".so";
	
	void* handle = dlopen(str.c_str(), RTLD_NOW);
	
	if (!handle) {
		log.error("Could not open library " + libName + ": " + dlerror());
	}
	
	return handle;
}

void* DllOpen::OpenDll(const std::string& libName, const std::string& filePath)
{
	std::string str = filePath + "lib" + libName + ".so";
	
	void* handle = dlopen(str.c_str(), RTLD_NOW);
	
	if (!handle) {
		log.error("Could not open library " + libName + ": " + dlerror());
	}
	
	return handle;
}

void* DllOpen::GetFunction(void* lib, const std::string& funcName)
{
	return dlsym(lib, funcName.c_str());
}

void DllOpen::CloseDll(void* lib)
{
	dlclose(lib);
}

#endif

} // namespace Kernel
} // namespace Mantid

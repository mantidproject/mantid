//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <iostream>
#include "MantidKernel/Support.h"

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
#else
#include "/usr/include/dlfcn.h"
#include "MantidKernel/DllOpen.h"
#endif

namespace Mantid
{
namespace Kernel
{
// Get a reference to the logger
Logger& DllOpen::log = Logger::get("DllOpen");

void* DllOpen::OpenDll(const std::string& libName)
{
	std::string str = LIB_PREFIX + libName + LIB_POSTFIX;
	return OpenDllImpl(str);
}

void* DllOpen::OpenDll(const std::string& libName, const std::string& filePath)
{
	std::string str = filePath + "/" + LIB_PREFIX + libName + LIB_POSTFIX;
	return OpenDllImpl(str);
}

void* DllOpen::GetFunction(void* lib, const std::string& funcName)
{
	return GetFunctionImpl(lib, funcName);
}

void DllOpen::CloseDll(void* lib)
{
	CloseDllImpl(lib);
}

/** Converts a file name (without directory) to a undecorated library name.
 * e.g. MyLibrary.dll or libMyLibary.so would become MyLibrary
 * @param fileName the filename (with extension) to convert
 * @returns the converted libName, or empty string if the conversion was not possible.
 */
const std::string DllOpen::ConvertToLibName(const std::string& fileName)
{
  //take a copy of the input string
  std::string retVal = fileName;

  if ((retVal.find(LIB_PREFIX) == 0 ) && 
    (retVal.find("\\") == std::string::npos) &&
    (retVal.find("/") == std::string::npos))
  {
    //found
    retVal = retVal.substr(LIB_PREFIX.size(),retVal.size()- LIB_PREFIX.size());
  }
  else
  {
    //prefix not found
    return "";
  }

  if ( retVal.rfind(LIB_POSTFIX) == (retVal.size()-LIB_POSTFIX.size()))
  {
    //found
    retVal = retVal.substr(0,retVal.size()-LIB_POSTFIX.size());
  }
  else
  {
    //postfix not found
    return "";
  }
  return retVal;
}

#if _WIN32
const std::string DllOpen::LIB_PREFIX = "";
const std::string DllOpen::LIB_POSTFIX = ".dll";

void* DllOpen::OpenDllImpl(const std::string& filePath)
{
	return LoadLibrary(filePath.c_str());
}

void* DllOpen::GetFunctionImpl(void* lib, const std::string& funcName)
{
	return GetProcAddress((HINSTANCE)lib, funcName.c_str());
}

void DllOpen::CloseDllImpl(void* lib)
{
	FreeLibrary((HINSTANCE)lib);
}
#else
const std::string DllOpen::LIB_PREFIX = "lib";
const std::string DllOpen::LIB_POSTFIX = ".so";

void* DllOpen::OpenDllImpl(const std::string& filePath)
{
	void* handle = dlopen(filePath.c_str(), RTLD_NOW);
	if (!handle) {
		log.error("Could not open library " + libName + ": " + dlerror());
	}
	return handle;
}

void* DllOpen::GetFunctionImpl(void* lib, const std::string& funcName)
{
	return dlsym(lib, funcName.c_str());
}

void DllOpen::CloseDllImpl(void* lib)
{
	dlclose(lib);
}

#endif

} // namespace Kernel
} // namespace Mantid

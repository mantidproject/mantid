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
#define _WIN32_WINNT 0x0510
#include <windows.h>
//#include <strsafe.h>
#else
#include <dlfcn.h>
#endif /* _WIN32 */

#include "MantidKernel/Strings.h"
#include "MantidKernel/DllOpen.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{

// Get a reference to the logger
Logger& DllOpen::log = Logger::get("DllOpen");

/* Opens the shared library after appending the required formatting,
 * i.e. libName.so for Linux and Name.dll for Windows.
 * Calls the correct implementation based on the current O/S.
 * @param libName :: Name of the library.
 * @return Pointer to library (of type void).
 **/
void* DllOpen::OpenDll(const std::string& libName)
{
  std::string str = LIB_PREFIX + libName + LIB_POSTFIX;
  return OpenDllImpl(str);
}

/* Opens the shared library after appending the required formatting,
 * i.e. libName.so for Linux and Name.dll for Windows.
 * Calls the correct implementation based on the current O/S.
 * @param libName :: Name of the library.
 * @param filePath :: The location on the library.
 * @return Pointer to library (of type void).
 **/
void* DllOpen::OpenDll(const std::string& libName, const std::string& filePath)
{
  std::string str = filePath + PATH_SEPERATOR + LIB_PREFIX + libName
    + LIB_POSTFIX;
  return OpenDllImpl(str);
}

/* Retrieves a function from the opened library.
 * Calls the correct implementation based on the current O/S.
 * @param libName :: Name of the library.
 * @param funcName :: The name of the function to retrieve.
 * @return Pointer to the function (of type void).
 **/
void* DllOpen::GetFunction(void* libName, const std::string& funcName)
{
  return GetFunctionImpl(libName, funcName);
}

/* Closes an open library.
 * Calls the correct implementation based on the current O/S.
 * @param libName :: Name of the library.
 **/
void DllOpen::CloseDll(void* libName)
{
  CloseDllImpl(libName);
}

/** Converts a file name (without directory) to a undecorated library name.
 * e.g. MyLibrary.dll or libMyLibary.so would become MyLibrary.
 * @param fileName :: The filename (with extension) to convert
 * @return The converted libName, or empty string if the conversion was not possible.
 **/
const std::string DllOpen::ConvertToLibName(const std::string& fileName)
{
  //take a copy of the input string
  std::string retVal = fileName;

  if ((retVal.find(LIB_PREFIX) == 0 ) && (retVal.find(PATH_SEPERATOR)
    == std::string::npos))
  {
    //found
    retVal = retVal.substr(LIB_PREFIX.size(), retVal.size()
      - LIB_PREFIX.size());
  }
  else
  {
    //prefix not found
    return "";
  }

  if (retVal.rfind(LIB_POSTFIX) == (retVal.size()-LIB_POSTFIX.size()))
  {
    //found
    retVal = retVal.substr(0, retVal.size()-LIB_POSTFIX.size());
  }
  else
  {
    //postfix not found
    return "";
  }
  return retVal;
}

/* Adds a directory to the dll cearch path
 **/
void DllOpen::addSearchDirectory(const std::string& dir)
{
  addSearchDirectoryImpl(dir);
}

#if _WIN32
const std::string DllOpen::LIB_PREFIX = "";
const std::string DllOpen::LIB_POSTFIX = ".dll";
const std::string DllOpen::PATH_SEPERATOR = "\\";

/* Opens the Windows .dll file.
 * @param filePath :: Filepath of the library.
 * @return Pointer to library (of type void).
 **/
void* DllOpen::OpenDllImpl(const std::string& filePath)
{
  void* handle = LoadLibrary(filePath.c_str());
  if (!handle)
  {

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );

    // Display the error message and exit the process
    size_t n = lstrlen((LPCTSTR)lpMsgBuf) + 40;

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, n*sizeof(TCHAR));
    //        StringCchPrintf((LPTSTR)lpDisplayBuf, 
    //          LocalSize(lpDisplayBuf) / sizeof(TCHAR),
    //          TEXT("failed with error %d: %s"), 
    //          dw, lpMsgBuf); 
    _snprintf((char*)lpDisplayBuf, n, "failed with error %d: %s", dw, lpMsgBuf);
    log.error()<<"Could not open library " << filePath << ": " << (LPCTSTR)lpDisplayBuf << std::endl;

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
  }
  return handle;
}

/* Retrieves a function from the opened .dll file.
 * Only works if the function has been declared as extern 'C'.
 * @param libName :: Name of the library.
 * @param funcName :: The name of the function to retrieve.
 * @return Pointer to the function (of type void).
 **/
void* DllOpen::GetFunctionImpl(void* libName, const std::string& funcName)
{
  return (void*)GetProcAddress((HINSTANCE)libName, funcName.c_str());
}

/* Closes an open .dll file.
 * @param libName :: Name of the library.
 **/
void DllOpen::CloseDllImpl(void* libName)
{
  FreeLibrary((HINSTANCE)libName);
}

/* Adds a directory to the dll cearch path
 **/
void DllOpen::addSearchDirectoryImpl(const std::string& dir)
{
  SetDllDirectory(dir.c_str());
}

#else

const std::string DllOpen::LIB_PREFIX = "lib";
// Shared libraries end in "so" on linux, "dylib" on the Mac
#ifdef __linux__
const std::string DllOpen::LIB_POSTFIX = ".so";
#elif defined __APPLE__
const std::string DllOpen::LIB_POSTFIX = ".dylib";
#endif

const std::string DllOpen::PATH_SEPERATOR = "/";

/* Opens the Linux .so file
 * @param filePath :: Filepath of the library.
 * @return Pointer to library (of type void).
 **/
void* DllOpen::OpenDllImpl(const std::string& filePath)
{
  void* handle = dlopen(filePath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!handle)
  {
    log.error("Could not open library " + filePath + ": " + dlerror());
  }
  return handle;
}

/* Retrieves a function from the opened library.
 * Only works if the function has been declared as extern 'C'.
 * @param libName :: Name of the library.
 * @param funcName :: The name of the function to retrieve.
 * @return Pointer to the function (of type void).
 **/
void* DllOpen::GetFunctionImpl(void* libName, const std::string& funcName)
{
  return dlsym(libName, funcName.c_str());
}

/* Closes an open .so file.
 * @param libName :: Name of the library.
 **/
void DllOpen::CloseDllImpl(void* libName)
{
  // Commented out for now due to a potential bug in glibc
  //dlclose(libName);
}

/* Adds a directory to the dll cearch path
 **/
void DllOpen::addSearchDirectoryImpl(const std::string& dir)
{
  (void) dir; //Avoid compiler warning
}

#endif /* _WIN32 */

} // namespace Kernel
} // namespace Mantid

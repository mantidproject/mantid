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
#include <strsafe.h>

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
      std::string str = filePath + PATH_SEPERATOR + LIB_PREFIX + libName + LIB_POSTFIX;
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
        (retVal.find(PATH_SEPERATOR) == std::string::npos))
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
    const std::string DllOpen::PATH_SEPERATOR = "\\";

    void* DllOpen::OpenDllImpl(const std::string& filePath)
    {
      void* handle =  LoadLibrary(filePath.c_str());
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

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
          (lstrlen((LPCTSTR)lpMsgBuf)+40)*sizeof(TCHAR)); 
        StringCchPrintf((LPTSTR)lpDisplayBuf, 
          LocalSize(lpDisplayBuf) / sizeof(TCHAR),
          TEXT("failed with error %d: %s"), 
          dw, lpMsgBuf); 

        log.error()<<"Could not open library " << filePath << ": " << lpDisplayBuf << std::endl;


        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
      }
      return handle;
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
    const std::string DllOpen::PATH_SEPERATOR = "/";

    void* DllOpen::OpenDllImpl(const std::string& filePath)
    {
      void* handle = dlopen(filePath.c_str(), RTLD_NOW);
      if (!handle) {
        log.error("Could not open library " + filePath + ": " + dlerror());
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

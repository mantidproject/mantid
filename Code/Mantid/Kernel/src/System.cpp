//-----------------------------
//Includes
//-----------------------------
#include "MantidKernel/System.h"
#include "Poco/Path.h"

// Need OS defined functions
#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

/**
 * Get the directory containing the program executable
 * @returns A string containing the path of the directory 
 * containing the executable, including a trailing slash
 */
std::string Mantid::Kernel::getDirectoryOfExecutable()
{
  std::string execpath("");
  const size_t LEN(1024);
  char pBuf[LEN];
  int bytes(0);
#ifdef _WIN32
  bytes = GetModuleFileName(NULL, pBuf, LEN);
#else
  char szTmp[32];
  sprintf(szTmp, "/proc/%d/exe", getpid());
  bytes = readlink(szTmp, pBuf, LEN);
#endif
  if( bytes > 0 && bytes < 1024 )
  {
    pBuf[bytes] = '\0';
    execpath = std::string(pBuf);
  }
  return Poco::Path(execpath).parent().toString();
}

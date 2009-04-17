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
  return Poco::Path(getPathToExecutable()).parent().toString();
}

/**
  * Get the  full path the the program executable. This is not necessarily MantidPlot or a main
  * program since if we are running through Python, it is the Python executable that is considered
  * as the executing program
  * @returns A string containing the full path the the executable
  */
std::string Mantid::Kernel::getPathToExecutable()
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
  return execpath;
}

/**
 * Check if the path is on a network drive
 * @param path The path to be checked
 * @return True if the path is on a network drive.
 */
bool Mantid::Kernel::isNetworkDrive(const std::string path)
{
#ifdef _WIN32
    // if path is relative get the full one
    char buff[MAX_PATH];
    GetFullPathName(path.c_str(),MAX_PATH,buff,NULL);
    std::string fullName(buff);
    size_t i = fullName.find(':');

    // if the full path doesn't contain a drive letter assume it's on the network
    if (i == std::string::npos) return true;

    fullName.erase(i+1);
    fullName += '\\';  // make sure the name has the trailing backslash
    UINT type = GetDriveType(fullName.c_str());
    return DRIVE_REMOTE == type;
#endif
    return false;
}

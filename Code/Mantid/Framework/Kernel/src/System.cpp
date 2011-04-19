//-----------------------------
//Includes
//-----------------------------
#include "MantidKernel/System.h"
#include <Poco/Path.h>
#include <boost/shared_ptr.hpp>
#include <climits>
#include <limits>
#include <cfloat>
#include <typeinfo>
#include <map>

// Need OS defined functions
#ifdef _WIN32
  #include <windows.h>
#elif defined __linux__
  #include <unistd.h>
  #include <fstream>
  #include <sstream>
  #include <algorithm>
  #include <iomanip>
  #include <iostream>
#elif defined __APPLE__
  #include <mach-o/dyld.h>
#endif

// MG 16/07/09: Some forward declarations of things from API. I need this so
// that the typeid function in getUnmangledTypeName knows about them
// This way I don't need to actually include the headers and I don't
// introduce unwanted dependencies
namespace Mantid
{
  namespace API
  {
    class Workspace;
    class MatrixWorkspace;
    class ITableWorkspace;
    class IMDEventWorkspace;
    class IMDWorkspace;
    class IEventWorkspace;
  }
  namespace DataObjects
  {
    class EventWorkspace;
    class PeaksWorkspace;
  }
  namespace MDDataObjects
  {
    class MDWorkspace;
  }
}

int Mantid::EMPTY_INT()
{
  return INT_MAX;
}

double Mantid::EMPTY_DBL()
{
  return DBL_MAX/2;
}

/// Constructor
Mantid::Kernel::RegistrationHelper::RegistrationHelper(int)
{
}

/**
 * Get the directory containing the program executable
 * @returns A string containing the path of the directory 
 * containing the executable, including a trailing slash
 */
std::string Mantid::Kernel::getDirectoryOfExecutable()
{
  //std::cout << "getDirectoryOfExecutable is " << Poco::Path(getPathToExecutable()).parent().toString() << std::endl
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
  
#ifdef _WIN32
  unsigned int bytes = GetModuleFileName(NULL, pBuf, LEN);
#elif defined __linux__
  char szTmp[32];
  sprintf(szTmp, "/proc/%d/exe", getpid());
  ssize_t bytes = readlink(szTmp, pBuf, LEN);
#elif defined __APPLE__
  // Two calls to _NSGetExecutablePath required - first to get size of buffer
  uint32_t bytes(0);
  _NSGetExecutablePath(pBuf,&bytes);
  const int success = _NSGetExecutablePath(pBuf,&bytes);
  if (success < 0) bytes = 1025;
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
 * @param path :: The path to be checked
 * @return True if the path is on a network drive.
 */
bool Mantid::Kernel::isNetworkDrive(const std::string & path)
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
#elif defined __linux__
  // This information is only present in the /proc/mounts file on linux. There are no drives on
  // linux only mount locations therefore the test will have to check the path against
  // entries in /proc/mounts to see if the filesystem type is NFS or SMB (any others ????)
  // Each line corresponds to a particular mounted location
  // 1st column - device name
  // 2nd column - mounted location
  // 3rd column - filesystem type commonly ext2, ext3 for hard drives and NFS or SMB for
  //              network locations

  std::ifstream mntfile("/proc/mounts");
  std::string txtread("");
  while( getline(mntfile, txtread) )
  {
    std::istringstream strm(txtread);
    std::string devname(""), mntpoint(""), fstype("");
    strm >> devname >> mntpoint >> fstype;
    if( !strm ) continue;
    // I can't be sure that the file system type is always lower case
    std::transform(fstype.begin(), fstype.end(), fstype.begin(), toupper);
    // Skip the current line if the file system isn't a network one
    if( fstype != "NFS" && fstype != "SMB" ) continue;
    // Now we have a line containing a network filesystem and just need to check if the path
    // supplied contains the mount location. There is a small complication in that the mount
    // points within the file have certain characters transformed into their octal 
    // representations, for example spaces->040.
    std::string::size_type idx = mntpoint.find("\\0");
    if( idx != std::string::npos ) 
    {
      std::string oct = mntpoint.substr(idx + 1, 3);
      strm.str(oct);
      int printch(-1);
      strm.setf( std::ios::oct, std::ios::basefield );  
      strm >> printch;
      if( printch != -1 )
      { 
        mntpoint = mntpoint.substr(0, idx) + static_cast<char>(printch) + mntpoint.substr(idx + 4);
      }
      // Search for this at the start of the path
      if( path.find(mntpoint) == 0 ) return true;
    }     
  }
  return false;
#else
  // Not yet implemented for the mac
  return false;
#endif
}

/**
 * Get the unmangled name of the given typestring for some common types that we use. Note that
 * this is just a lookup and NOT an unmangling algorithm
 * @param type :: A pointer to the type_info object for this type
 * @returns An unmangled version of the name
 */
std::string  Mantid::Kernel::getUnmangledTypeName(const std::type_info& type)
{
  // Compile a lookup table. This is a static local variable that
  // will get initialized when the function is first used
  static std::map<std::string, std::string> typestrings;
  if( typestrings.empty() ) 
  {
    typestrings.insert(std::make_pair(typeid(char).name(), std::string("letter")));
    typestrings.insert(std::make_pair(typeid(int).name(), std::string("number")));
    typestrings.insert(std::make_pair(typeid(long long).name(), std::string("number")));
    typestrings.insert(std::make_pair(typeid(double).name(), std::string("number")));
    typestrings.insert(std::make_pair(typeid(bool).name(), std::string("boolean")));
    typestrings.insert(std::make_pair(typeid(std::string).name(), std::string("string")));
    typestrings.insert(std::make_pair(typeid(std::vector<std::string>).name(), std::string("str list")));
    typestrings.insert(std::make_pair(typeid(std::vector<int>).name(), std::string("int list")));
    typestrings.insert(std::make_pair(typeid(std::vector<double>).name(), std::string("dbl list")));

    //Workspaces
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::Workspace>).name(), std::string("Workspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::MatrixWorkspace>).name(), std::string("MatrixWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::ITableWorkspace>).name(), std::string("TableWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::IMDWorkspace>).name(), std::string("IMDWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::IMDEventWorkspace>).name(), std::string("MDEventWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::API::IEventWorkspace>).name(), std::string("IEventWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::DataObjects::EventWorkspace>).name(), std::string("EventWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::DataObjects::PeaksWorkspace>).name(), std::string("PeaksWorkspace")));
    typestrings.insert(std::make_pair(typeid(boost::shared_ptr<Mantid::MDDataObjects::MDWorkspace>).name(), std::string("MDWorkspace")));


  }
  std::map<std::string, std::string>::const_iterator mitr = typestrings.find(type.name());
  if( mitr != typestrings.end() )
  {
    return mitr->second;
  }

  return type.name();

}

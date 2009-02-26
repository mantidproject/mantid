#include <iomanip>
#include <iostream>
#include <vector>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <fstream>
#include <sstream>

using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{

/// Private Constructor for singleton class
MemoryManagerImpl::MemoryManagerImpl() :
  g_log(Kernel::Logger::get("MemoryManager"))
{
  std::cerr << "Memory Manager created." << std::endl;
  g_log.debug() << "Memory Manager created." << std::endl;
}

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed 
 *  out by Instance
 */
MemoryManagerImpl::~MemoryManagerImpl()
{
}

MemoryInfo MemoryManagerImpl::getMemoryInfo()
{
  MemoryInfo mi;
#ifdef _WIN32
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx( &memStatus );
  if (memStatus.ullTotalPhys < memStatus.ullTotalVirtual)
  {
    mi.availMemory = memStatus.ullAvailPhys/1024;
    mi.totalMemory = memStatus.ullTotalPhys/1024;
  }
  else// All virtual memory will be physical, but a process cannot have more than TotalVirtual.

  {
    mi.availMemory = memStatus.ullAvailVirtual/1024;
    mi.totalMemory = memStatus.ullTotalVirtual/1024;
  }
  mi.freeRatio = int(100*double(mi.availMemory)/mi.totalMemory);
#else

  /**
   * As usual things are more complex on Linux. I think we need to take into account
   * the value of Cached as well since, especially if the system has been running for a long time,
   * MemFree will seem a lot smaller than it should be.
   *
   * The only way I can see as to get acces to the Cached value is from the /proc/meminfo file
   * so if this is not successful I'll fall back to using the sysconf method and forget the cache
   */
  if (!ReadMemInfo(mi))
  {
    long int totPages = sysconf(_SC_PHYS_PAGES);
    long int avPages = sysconf(_SC_AVPHYS_PAGES);
    long int pageSize = sysconf(_SC_PAGESIZE);
    mi.availMemory = avPages / 1024 * pageSize;
    mi.totalMemory = totPages / 1024 * pageSize;
    mi.freeRatio = int(100 * double(mi.availMemory) / mi.totalMemory);
  }
#endif
  return mi;
}

#ifndef _WIN32
/**
 * A unix specific function to read values from /proc/meminfo
 * @param mi A struct to fill with the appropriate values
 */
bool MemoryManagerImpl::ReadMemInfo(Mantid::API::MemoryInfo & mi)
{
  std::ifstream file("/proc/meminfo");
  std::string line;
  int values_found(0);
  //Need to set this to zero
  mi.availMemory = 0;
  while (getline(file, line))
  {
    std::istringstream is(line);
    std::string tag;
    long value(0);
    is >> tag >> value;
    if (!is)
      return false;
    if (tag == "MemTotal:")
    {
      ++values_found;
      mi.totalMemory = value;
      g_log.debug() << "Linux - Total memory available: " << value << " KB.\n";
    }
    else if (tag == "MemFree:")
    {
      ++values_found;
      mi.availMemory += value;
      g_log.debug() << "Linux - Free memory reported: " << value << " KB.\n";
    }
    else if (tag == "Cached:")
    {
      ++values_found;
      mi.availMemory += value * 0.8;
      g_log.debug() << "Linux - Cached memory reported: " << value
          << " KB. Note: Using 80% of this value as additional free memory.\n";
    }
    else
      continue;
    if (values_found == 3)
    {
      mi.freeRatio = static_cast<int> (double(mi.availMemory) / mi.totalMemory * 100);
      g_log.debug() << "Linux - Memory taken to be available for use (incl. cache): " << mi.availMemory
          << " KB.\n";
      g_log.debug() << "Linux - Percentage of memory taken to be available for use (incl. cache): "
          << mi.freeRatio << "%.\n";
      file.close();
      return true;
    }
  }
  file.close();
  return false;
}
#endif

/** Decides if a ManagedWorkspace2D sould be created for the current memory conditions
 and workspace parameters NVectors, XLength,and YLength.
 */
bool MemoryManagerImpl::goForManagedWorkspace(int NVectors, int XLength, int YLength)
{
  int AlwaysInMemory;// Check for disabling flag
  if (Kernel::ConfigService::Instance().getValue("ManagedWorkspace.AlwaysInMemory", AlwaysInMemory)
      && AlwaysInMemory)
    return false;

  // check potential size to create and determine trigger  
  int availPercent;
  if (!Kernel::ConfigService::Instance().getValue("ManagedWorkspace.LowerMemoryLimit", availPercent))
  {
    // Default to 40% if missing
    availPercent = 40;
  }
  if (availPercent > 150)
  {
    g_log.warning("ManagedWorkspace.LowerMemoryLimit is not allowed to be greater than 150%.");
    availPercent = 150;
  }
  if (availPercent < 0)
  {
    g_log.warning("Negative value for ManagedWorkspace.LowerMemoryLimit. Setting to 0.");
    availPercent = 0;
  }
  if (availPercent > 90)
  {
    g_log.warning("ManagedWorkspace.LowerMemoryLimit is greater than 90%. Danger of memory errors.");
  }
  MemoryInfo mi = getMemoryInfo();
  int triggerSize = mi.availMemory / 100 * availPercent / sizeof(double);
  int wsSize = NVectors * (YLength * 2 + XLength) / 1024;
  g_log.debug() << "Requested memory: " << wsSize * sizeof(double) << " KB.\n";
  g_log.debug() << "Available memory: " << mi.availMemory << " KB.\n";
  g_log.debug() << "MWS trigger memory: " << triggerSize * sizeof(double) << " KB.\n";
  return wsSize > triggerSize;
}

} // namespace API
} // namespace Mantid

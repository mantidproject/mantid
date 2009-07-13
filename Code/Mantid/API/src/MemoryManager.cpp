#include <iomanip>
#include <iostream>
#include <limits>

#ifdef __linux__
#include <unistd.h>
#include <malloc.h>
#endif

#include "MantidAPI/MemoryManager.h"
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
#ifdef _WIN32
  memStatus.dwLength = sizeof(MEMORYSTATUSEX);
#endif
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
  GlobalMemoryStatusEx( &memStatus );

  if (memStatus.ullTotalPhys < memStatus.ullTotalVirtual)
  {
    mi.availMemory = static_cast<int>(memStatus.ullAvailPhys/1024);
    mi.totalMemory = static_cast<int>(memStatus.ullTotalPhys/1024);
  }
  else// All virtual memory will be physical, but a process cannot have more than TotalVirtual.
  {
    mi.availMemory = static_cast<int>(memStatus.ullAvailVirtual/1024);
    mi.totalMemory = static_cast<int>(memStatus.ullTotalVirtual/1024);
  }

#elif defined __linux__

  /*
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
  }
  // Can get the info on the memory that we've already obtained but aren't using right now
  const int unusedReserved = mallinfo().fordblks/1024;
  g_log.debug() << "Linux - Adding reserved but unused memory of " << unusedReserved << " KB\n";
  mi.availMemory += unusedReserved;
#else // Currently get to here if building on a mac. For now just use big numbers.
  mi.availMemory = 9000000;
  mi.totalMemory = 10000000;
#endif
    
  mi.freeRatio = static_cast<int>(100.0*mi.availMemory/mi.totalMemory);
  g_log.debug() << "Percentage of memory taken to be available for use (incl. cache): "
      << mi.freeRatio << "%.\n";
  return mi;
}

#ifdef __linux__
/**
 * A unix specific function to read values from /proc/meminfo
 * @param mi A struct to fill with the appropriate values
 */
bool MemoryManagerImpl::ReadMemInfo(MemoryInfo & mi)
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
      mi.availMemory += (8*value/10);
      g_log.debug() << "Linux - Cached memory reported: " << value
          << " KB. Note: Using 80% of this value as additional free memory.\n";
    }
    else
      continue;
    if (values_found == 3)
    {
      g_log.debug() << "Linux - Memory taken to be available for use (incl. cache): " << mi.availMemory
          << " KB.\n";
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
    @param NVectors the number of vectors
    @param XLength the size of the X vector
    @param YLength the size of the Y vector
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
  unsigned int triggerSize = mi.availMemory / 100 * availPercent / sizeof(double);
  // Avoid int overflow
  unsigned int wsSize;
  if (NVectors > 1024)
      wsSize = NVectors / 1024 * (YLength * 2 + XLength);
  else if (YLength * 2 + XLength > 1024)
      wsSize = (YLength * 2 + XLength) / 1024 * NVectors;
  else
      NVectors * (YLength * 2 + XLength) / 1024;

  bool goManaged = (wsSize > triggerSize);
#ifdef _WIN32
  // If we're on the cusp of going managed, add in the reserved but unused memory
  if (goManaged)
  {
    triggerSize += ReservedMem() / 100 * availPercent / sizeof(double);
    goManaged = (wsSize > triggerSize);
  }
#endif

  g_log.debug() << "Requested memory: " << wsSize * sizeof(double) << " KB.\n";
  g_log.debug() << "Available memory: " << mi.availMemory << " KB.\n";
  g_log.debug() << "MWS trigger memory: " << triggerSize * sizeof(double) << " KB.\n";

  return goManaged;
}

#ifdef _WIN32
/// Returns the reserved, but currently unused, memory in KB (Windows only) 
unsigned int MemoryManagerImpl::ReservedMem()
{
  MEMORY_BASIC_INFORMATION info; // Windows structure

  char *addr = NULL;
  size_t unusedReserved = 0; // total reserved space
  DWORDLONG size = 0;
  DWORDLONG GB2 = memStatus.ullTotalVirtual; // Maximum memory available to the process

  // Loop over all virtual memory to find out the status of every block.
  do
  {
    VirtualQuery(addr,&info,sizeof(MEMORY_BASIC_INFORMATION));
        
    // Count up the total size of reserved but unused blocks
    if (info.State == MEM_RESERVE) unusedReserved += info.RegionSize;

    addr += info.RegionSize; // Move up to the starting address for the next call
    size += info.RegionSize;
  } 
  while(size < GB2);

  // Convert from bytes to KB
  unusedReserved /= 1024;

  g_log.debug() << "Windows - Adding reserved but unused memory of " << unusedReserved << " KB\n";

  return unusedReserved;
}
#endif

} // namespace API
} // namespace Mantid

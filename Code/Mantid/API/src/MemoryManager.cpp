#include <iomanip>
#include <iostream>
#include <limits>

#ifdef __linux__
#include <unistd.h>
#include <malloc.h>
#elif __APPLE__
#include <malloc/malloc.h>
#include <sys/sysctl.h>
#include <mach/mach_host.h>
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

  // Try to enable the Low Fragmentation Heap for all heaps
  // Bit of a brute force approach, but don't know which heap workspace data end up on
  HANDLE hHeaps[1025];
  // Get the number of heaps
  const DWORD numHeap = GetProcessHeaps(1024, hHeaps);
  g_log.debug() << "Number of heaps: " << GetProcessHeaps(0, NULL) << "\n";
  ULONG ulEnableLFH = 2; // 2 = Low Fragmentation Heap
  for(DWORD i = 0; i < numHeap; i++)
  {
    if(!HeapSetInformation(hHeaps[i], HeapCompatibilityInformation, &ulEnableLFH, sizeof(ulEnableLFH)))
    {
      g_log.debug() << "Failed to enable the LFH for heap " << i << "\n";
    }
  }
#endif
  
#ifdef __linux__
  /* The line below tells malloc to use a different memory allocation system call (mmap) to the 'usual'
   * one (sbrk) for requests above the threshold of the second argument (in bytes). The effect of this 
   * is that, for the current threshold value of 8*4096, storage for workspaces having 4096 or greater
   * bins per spectrum will be allocated using mmap.
   * This should have the effect that memory is returned to the kernel as soon as a workspace is deleted,
   * preventing things going to managed workspaces when they shouldn't. This will also hopefully reduce
   * memory fragmentation.
   * Potential downsides to look out for are whether this memory allocation technique makes things
   * noticeably slower and whether it wastes memory (mmap allocates in blocks of the system page size.
   */
  mallopt(M_MMAP_THRESHOLD, 8*4096);
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

#elif defined __APPLE__

    // Get the total RAM of the system
    uint64_t totalmem;
    size_t len = sizeof(totalmem);
    // Gives system memory in bytes
    int err = sysctlbyname("hw.memsize",&totalmem,&len,NULL,0);
    if (err) g_log.warning("Unable to obtain memory of system");
    mi.totalMemory = totalmem / 1024;

    // Now get the amount of free memory (=free+inactive memory)
    mach_port_t port = mach_host_self();
    // Need to find out the system page size for next part
    vm_size_t pageSize;
    host_page_size(port, &pageSize);
    vm_statistics vmStats;
    mach_msg_type_number_t count;
    count = sizeof(vm_statistics) / sizeof(natural_t);
    err = host_statistics(port, HOST_VM_INFO, (host_info_t)&vmStats, &count);
    if (err) g_log.warning("Unable to obtain memory statistics");
    mi.availMemory = pageSize * ( vmStats.free_count + vmStats.inactive_count ) / 1024;

    // Now add in reserved but unused memory as reported by malloc
    const size_t unusedReserved = mstats().bytes_free / 1024;
    g_log.debug() << "Mac - Adding reserved but unused memory of " << unusedReserved << " KB\n";
    mi.availMemory += unusedReserved;

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
    @param isCompressedOK The address of a boolean indicating if the compression succeeded or not
 */
bool MemoryManagerImpl::goForManagedWorkspace(int NVectors, int XLength, int YLength, bool* isCompressedOK)
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
  unsigned int wsSize = 0;
  if (NVectors > 1024)
      wsSize = NVectors / 1024 * (YLength * 2 + XLength);
  else if (YLength * 2 + XLength > 1024)
      wsSize = (YLength * 2 + XLength) / 1024 * NVectors;
  else
      wsSize = NVectors * (YLength * 2 + XLength) / 1024;

  bool goManaged = (wsSize > triggerSize);
#ifdef _WIN32
  // If we're on the cusp of going managed, add in the reserved but unused memory
  if (goManaged)
  {
    triggerSize += ReservedMem() / 100 * availPercent / sizeof(double);
    goManaged = (wsSize > triggerSize);
  }
#endif

  if (isCompressedOK)
  {
    if (goManaged)
    {
      int notOK = 0;
      if ( !Kernel::ConfigService::Instance().getValue("CompressedWorkspace.DoNotUse",notOK) ) notOK = 0;
      if (notOK) *isCompressedOK = false;
      else
      {
        double compressRatio;
        if (!Kernel::ConfigService::Instance().getValue("CompressedWorkspace.EstimatedCompressRatio",compressRatio)) compressRatio = 4.;
        int VectorsPerBlock;
        if (!Kernel::ConfigService::Instance().getValue("CompressedWorkspace.VectorsPerBlock",VectorsPerBlock)) VectorsPerBlock = 4;
        double compressedSize = (1./compressRatio + 100.0*VectorsPerBlock/NVectors) * wsSize;
        double memoryLeft = (double(triggerSize)/availPercent*100 - compressedSize)/1024 * sizeof(double);
        // To prevent bad allocation on Windows when free memory is too low.
        if (memoryLeft < 200.)
          *isCompressedOK = false;
        else
          *isCompressedOK =  compressedSize < double(triggerSize);
      }
    }
    else
    {
      *isCompressedOK = false;
    }
  }

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

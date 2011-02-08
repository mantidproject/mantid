//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Memory.h"

#ifdef USE_TCMALLOC
#include "google/malloc_extension.h"
#endif

using std::size_t;

namespace Mantid
{
namespace API
{

/// Private Constructor for singleton class
MemoryManagerImpl::MemoryManagerImpl() :
  g_log(Kernel::Logger::get("MemoryManager"))
{
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
  Kernel::MemoryStats mem_stats;
  MemoryInfo info;
  info.totalMemory = mem_stats.totalMem();
  info.availMemory = mem_stats.availMem();
  info.freeRatio = static_cast<size_t>(mem_stats.getFreeRatio());

  return info;
}

#ifdef _WIN32
namespace {  // Anonymous namespace

  MEMORYSTATUSEX memStatusWIN; ///< A Windows structure holding information about memory usage

  size_t ReservedMem()
  {
    MEMORY_BASIC_INFORMATION info; // Windows structure

    char *addr = NULL;
    size_t unusedReserved = 0; // total reserved space
    DWORDLONG size = 0;
    DWORDLONG GB2 = memStatusWIN.ullTotalVirtual; // Maximum memory available to the process

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

    return unusedReserved;
  }

}
#endif


/** Decides if a ManagedWorkspace2D sould be created for the current memory conditions
    and workspace parameters NVectors, XLength,and YLength.
    @param NVectors :: the number of vectors
    @param XLength :: the size of the X vector
    @param YLength :: the size of the Y vector
    @param isCompressedOK :: The address of a boolean indicating if the compression succeeded or not
    @return true is managed workspace is needed
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
  // We do this here for windows (rather than always including it in the calculation)
  // because the function called is, in principle, potentially quite expensive.
  if (goManaged)
  {
    const size_t reserved = ReservedMem();
    g_log.debug() << "Windows - Adding reserved but unused memory of " << reserved << " KB\n";
    triggerSize += reserved / 100 * availPercent / sizeof(double);
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


/** Release any free memory back to the system.
 * Calling this could help the system avoid going into swap.
 * NOTE: This only works if you linked against tcmalloc.
 */
void MemoryManagerImpl::releaseFreeMemory()
{

#ifdef USE_TCMALLOC
//    Kernel::MemoryStats mem;
//    mem.update();
//    std::cout << "Before releasing: " << mem << "\n";

    // Make TCMALLOC release memory to the system
    MallocExtension::instance()->ReleaseFreeMemory();

//    mem.update();
//    std::cout << "After releasing:  " << mem << "\n";
#endif
}

} // namespace API
} // namespace Mantid

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
  g_log(Kernel::Logger::get("MemoryManager")),
  memoryCleared(0)
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

  g_log.debug() << "Requested memory: " << wsSize * sizeof(double) << " KB.\n";
  g_log.debug() << "Available memory: " << mi.availMemory << " KB.\n";
  g_log.debug() << "MWS trigger memory: " << triggerSize * sizeof(double) << " KB.\n";

  bool goManaged = (wsSize > triggerSize);
  // If we're on the cusp of going managed, add in the reserved but unused memory
  if( goManaged )
  {
    // This is called separately as on some systems it is an expensive calculation.
    // See Kernel/src/Memory.cpp - reservedMem() for more details
    Kernel::MemoryStats mem_stats;
    const size_t reserved = mem_stats.reservedMem();
    g_log.debug() << "Windows - Adding reserved but unused memory of " << reserved << " KB\n";
    mi.availMemory += reserved;
    triggerSize += reserved / 100 * availPercent / sizeof(double);
    goManaged = (wsSize > triggerSize);

    g_log.debug() << "Available memory: " << mi.availMemory << " KB.\n";
    g_log.debug() << "MWS trigger memory: " << triggerSize * sizeof(double) << " KB.\n";
  }

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

/** Release any free memory back to the system,
 * but only if you are above a certain fraction of use of the
 * total available PHYSICAL memory.
 * Calling this could help the system avoid going into swap.
 *
 * NOTE: This only works if you linked against tcmalloc.
 * NOTE 2: This takes at least 0.1 ms on a Ubuntu 10.10 system, because of
 *      the call to MemoryStats.update().
 *
 * @param threshold :: multiplier (0-1.0) of the amount of physical
 *        memory used that has to be in use before the call to
 *        release memory is actually called.
 */
void MemoryManagerImpl::releaseFreeMemoryIfAbove(double threshold)
{

#ifdef USE_TCMALLOC
    Kernel::MemoryStats mem;
    mem.update();
    double fraction_available = (mem.availMem() * 1.0) / (mem.totalMem() * 1.0);
    if (fraction_available < (1.0 - threshold))
    {
      // Make TCMALLOC release memory to the system
      MallocExtension::instance()->ReleaseFreeMemory();
    }
#endif
}


/** Release memory back to the system if you accumulated enough.
 * Each call adds to the amount cleared, but it is only
 * released if past the threshold.
 * NOTE: This only works if you linked against tcmalloc.
 *
 * @param adding :: how many bytes where just cleared
 * @param threshold :: threshold number of bytes accumulated before clearing memory.
 */
void MemoryManagerImpl::releaseFreeMemoryIfAccumulated(size_t adding, size_t threshold)
{
#ifdef USE_TCMALLOC
  accumulatorMutex.lock();

  memoryCleared += adding;
  if (memoryCleared > threshold)
  {
    releaseFreeMemory();
    memoryCleared = 0;
  }

  accumulatorMutex.unlock();
#endif
}


} // namespace API
} // namespace Mantid

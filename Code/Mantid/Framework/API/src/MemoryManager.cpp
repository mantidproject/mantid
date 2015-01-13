//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Memory.h"

#include <ostream> //for endl

using std::size_t;

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("MemoryManager");
}

/// Private Constructor for singleton class
MemoryManagerImpl::MemoryManagerImpl() : memoryCleared(0) {
  g_log.debug() << "Memory Manager created." << std::endl;
}

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed
 *  out by Instance
 */
MemoryManagerImpl::~MemoryManagerImpl() {}

MemoryInfo MemoryManagerImpl::getMemoryInfo() {
  Kernel::MemoryStats mem_stats;
  MemoryInfo info;
  info.totalMemory = mem_stats.totalMem();
  info.availMemory = mem_stats.availMem();
  info.freeRatio = static_cast<size_t>(mem_stats.getFreeRatio());
  return info;
}

/** Release any free memory back to the system.
 * Calling this could help the system avoid going into swap.
 * NOTE: This only works if you linked against tcmalloc.
 */
void MemoryManagerImpl::releaseFreeMemory() {}

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
void MemoryManagerImpl::releaseFreeMemoryIfAbove(double threshold) {
  UNUSED_ARG(threshold);
}

/** Release memory back to the system if you accumulated enough.
 * Each call adds to the amount cleared, but it is only
 * released if past the threshold.
 * NOTE: This only works if you linked against tcmalloc.
 *
 * @param adding :: how many bytes where just cleared
 * @param threshold :: threshold number of bytes accumulated before clearing
 *memory.
 */
void MemoryManagerImpl::releaseFreeMemoryIfAccumulated(size_t adding,
                                                       size_t threshold) {
  UNUSED_ARG(adding);
  UNUSED_ARG(threshold);
}

} // namespace API
} // namespace Mantid

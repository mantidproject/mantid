#ifndef MANTID_KERNEL_MEMORY_H_
#define MANTID_KERNEL_MEMORY_H_

#include <string>
#include "MantidKernel/DllExport.h"

namespace Mantid
{
namespace Kernel
{

enum DLLExport MemoryStatsIgnore{MEMORY_STATS_IGNORE_NONE, MEMORY_STATS_IGNORE_SYSTEM, MEMORY_STATS_IGNORE_PROCESS};

class DLLExport MemoryStats
{
public:
  MemoryStats(const MemoryStatsIgnore ignore=MEMORY_STATS_IGNORE_NONE);
  void update();
  void ignoreFields(const MemoryStatsIgnore);
  std::string vmUsageStr() const;
  std::string resUsageStr() const;
  std::string totalMemStr() const;
  std::string availMemStr() const;
  std::size_t totalMem() const;
  std::size_t availMem() const;
  double getFreeRatio() const;
private:
  MemoryStatsIgnore ignore; ///< What fields to ignore.
  std::size_t vm_usage; ///< Virtual memory usage by process in kiB.
  std::size_t res_usage; ///< Resident memory usage by process in kiB.
  std::size_t total_memory; ///< Total physical memory of system in kiB.
  std::size_t avail_memory; ///< Available memory of system in kiB.
  friend DLLExport std::ostream& operator<<(std::ostream& out, const MemoryStats &stats);
};

DLLExport std::ostream& operator<<(std::ostream& out, const MemoryStats &stats);

/// Convert a (number) for memory in kiB to a string with proper units.
template <typename TYPE>
std::string memToString(const TYPE mem_in_kiB);


} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNELMEMORY_H_ */

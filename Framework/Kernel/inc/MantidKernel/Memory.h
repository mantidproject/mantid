// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <iosfwd>
#include <mutex>
#include <string>

namespace Mantid {
namespace Kernel {

/// Enmuerate the ignored memory fields
enum MemoryStatsIgnore { MEMORY_STATS_IGNORE_NONE, MEMORY_STATS_IGNORE_SYSTEM, MEMORY_STATS_IGNORE_PROCESS };
namespace MemoryOptions {
/// Initialize platform-dependent options for memory management
MANTID_KERNEL_DLL void initAllocatorOptions();
} // namespace MemoryOptions

/**
This class is responsible for memory statistics.
*/
class MANTID_KERNEL_DLL MemoryStats {
public:
  MemoryStats(const MemoryStatsIgnore ignore = MEMORY_STATS_IGNORE_NONE);
  void update();
  void ignoreFields(const MemoryStatsIgnore);
  std::string vmUsageStr() const;
  std::string resUsageStr() const;
  std::string totalMemStr() const;
  std::string availMemStr() const;
  std::size_t totalMem() const;
  std::size_t availMem() const;
  std::size_t residentMem() const;
  std::size_t virtualMem() const;
  std::size_t reservedMem() const;
  std::size_t getCurrentRSS() const;
  std::size_t getPeakRSS() const;
  double getFreeRatio() const;

private:
  void process_mem_system(size_t &sys_avail, size_t &sys_total);
  MemoryStatsIgnore ignore; ///< What fields to ignore.
  std::size_t vm_usage;     ///< Virtual memory usage by process in kiB.
  std::size_t res_usage;    ///< Resident memory usage by process in kiB.
  std::size_t total_memory; ///< Total physical memory of system in kiB.
  std::size_t avail_memory; ///< Available memory of system in kiB.
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out, const MemoryStats &stats);
  /// Mutex to avoid simultaneous access to memory resources
  static std::mutex mutexMemory;
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out, const MemoryStats &stats);

/// Convert a (number) for memory in kiB to a string with proper units.
template <typename TYPE> std::string memToString(const TYPE mem_in_kiB);

} // namespace Kernel
} // namespace Mantid

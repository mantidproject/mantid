#ifndef MANTID_KERNEL_MEMORY_H_
#define MANTID_KERNEL_MEMORY_H_

#include <string>
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace Kernel {

/// Enmuerate the ignored memory fields
enum MemoryStatsIgnore {
  MEMORY_STATS_IGNORE_NONE,
  MEMORY_STATS_IGNORE_SYSTEM,
  MEMORY_STATS_IGNORE_PROCESS
};
namespace MemoryOptions {
/// Initialize platform-dependent options for memory management
MANTID_KERNEL_DLL void initAllocatorOptions();
}

/**
This class is responsible for memory statistics.

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
  double getFreeRatio() const;

private:
  void process_mem_system(size_t &sys_avail, size_t &sys_total);
  MemoryStatsIgnore ignore; ///< What fields to ignore.
  std::size_t vm_usage;     ///< Virtual memory usage by process in kiB.
  std::size_t res_usage;    ///< Resident memory usage by process in kiB.
  std::size_t total_memory; ///< Total physical memory of system in kiB.
  std::size_t avail_memory; ///< Available memory of system in kiB.
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out,
                                                    const MemoryStats &stats);
  /// Mutex to avoid simultaneous access to memory resources
  static Mutex mutexMemory;
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out,
                                           const MemoryStats &stats);

/// Convert a (number) for memory in kiB to a string with proper units.
template <typename TYPE> std::string memToString(const TYPE mem_in_kiB);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNELMEMORY_H_ */

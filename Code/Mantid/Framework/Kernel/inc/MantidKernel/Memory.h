#ifndef MANTID_KERNEL_MEMORY_H_
#define MANTID_KERNEL_MEMORY_H_

#include <string>
#include "MantidKernel/DllExport.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
  namespace Kernel
  {
    /// Enmuerate the ignored memory fields

    enum DLLExport MemoryStatsIgnore{MEMORY_STATS_IGNORE_NONE, MEMORY_STATS_IGNORE_SYSTEM, MEMORY_STATS_IGNORE_PROCESS};
    namespace MemoryOptions
    {
      /// Initialize platform-dependent options for memory management
      DLLExport void initAllocatorOptions();
    }

    /** 
    This class is responsible for memory statistics.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
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
      std::size_t reservedMem() const;
      double getFreeRatio() const;
    private:
      void process_mem_system(size_t & sys_avail, size_t & sys_total);
      MemoryStatsIgnore ignore; ///< What fields to ignore.
      std::size_t vm_usage; ///< Virtual memory usage by process in kiB.
      std::size_t res_usage; ///< Resident memory usage by process in kiB.
      std::size_t total_memory; ///< Total physical memory of system in kiB.
      std::size_t avail_memory; ///< Available memory of system in kiB.
      static Logger &g_log; ///< Logger
      friend DLLExport std::ostream& operator<<(std::ostream& out, const MemoryStats &stats);
      /// Mutex to avoid simultaneous access to memory resources
      static Mutex mutexMemory;
    };

    DLLExport std::ostream& operator<<(std::ostream& out, const MemoryStats &stats);

    /// Convert a (number) for memory in kiB to a string with proper units.
    template <typename TYPE>
    std::string memToString(const TYPE mem_in_kiB);


  } // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNELMEMORY_H_ */

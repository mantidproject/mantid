#include "MantidKernel/Memory.h"
#include "MantidKernel/Logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <fstream>
#include <malloc.h>
#endif
#ifdef __APPLE__
#include <malloc/malloc.h>
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <Psapi.h>
#endif

using std::size_t;
using std::string;

namespace Mantid {
namespace Kernel {
namespace {
/// static logger object
Logger g_log("Memory");
}

/// Utility function to convert memory in kiB into easy to read units.
template <typename TYPE> string memToString(const TYPE mem_in_kiB) {
  std::stringstream buffer;
  if (mem_in_kiB < static_cast<TYPE>(1024))
    buffer << mem_in_kiB << " kB";
  else if (mem_in_kiB < static_cast<TYPE>(100 * 1024 * 1024))
    buffer << (mem_in_kiB / static_cast<TYPE>(1024)) << " MB";
  else
    buffer << (mem_in_kiB / static_cast<TYPE>(1024 * 1024)) << " GB";
  return buffer.str();
}

// -------------------- functions for getting the memory associated with the
// process
/** Attempts to read the system-dependent data for a process' virtual memory
 * size and resident set size, and return the results in KB. On failure, returns
 * 0.0, 0.0
 * @param vm_usage :: The virtual memory usage is stored in this variable in KiB
 * @param resident_set:: The memory associated with the current process in KiB
 */
void process_mem_usage(size_t &vm_usage, size_t &resident_set) {
  vm_usage = 0;
  resident_set = 0;

#ifdef __linux__
  // Adapted from
  // http://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-run-time-in-c
  using std::ios_base;
  using std::ifstream;

  // 'file' stat seems to give the most reliable results
  ifstream stat_stream("/proc/self/stat", ios_base::in);

  // dummy vars for leading entries in stat that we don't care about
  string pid, comm, state, ppid, pgrp, session, tty_nr;
  string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  string utime, stime, cutime, cstime, priority, nice;
  string O, itrealvalue, starttime;

  // the two fields we want
  unsigned long vsize; // according to man this is %lu
  long rss;            // according to man this is %ld

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >>
      tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >>
      stime >> cutime >> cstime >> priority >> nice >> O >> itrealvalue >>
      starttime >> vsize >> rss; // don't care about the rest

  long page_size_kb = sysconf(_SC_PAGE_SIZE) /
                      1024; // in case x86-64 is configured to use 2MB pages
  vm_usage = static_cast<size_t>(vsize / static_cast<long double>(1024.0));
  resident_set = static_cast<size_t>(rss * page_size_kb);
#elif __APPLE__
  // Adapted from http://blog.kuriositaet.de/?p=257. No official apple docs
  // could be found
  // task_t task = MACH_PORT_NULL;
  struct task_basic_info t_info;
  mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

  if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO,
                                (task_info_t)&t_info, &t_info_count)) {
    return;
  }
  // Need to find out the system page size for next part
  vm_size_t pageSize;
  mach_port_t port = mach_host_self();
  host_page_size(port, &pageSize);
  resident_set = static_cast<size_t>(t_info.resident_size * pageSize);
  vm_usage = static_cast<size_t>(t_info.virtual_size * pageSize / 1024.0);
#elif _WIN32
  // Adapted from
  // http://msdn.microsoft.com/en-us/library/windows/desktop/ms682050%28v=vs.85%29.aspx
  DWORD pid = GetCurrentProcessId();
  HANDLE hProcess =
      OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (NULL == hProcess)
    return;
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
    vm_usage = pmc.PagefileUsage / 1024;
    resident_set = pmc.WorkingSetSize / 1024;
  }
  CloseHandle(hProcess);
#endif
}

// ----------------------- functions associated with getting the memory of the
// system

#ifdef __linux__
/**
 * This function reads /proc/meminfo to get the system information.
 * @param sys_avail :: An output variable containing the available system memory
 * in KiB
 * @param sys_total :: An output variable containing the total system memory in
 * KiB
 */
bool read_mem_info(size_t &sys_avail, size_t &sys_total) {
  std::ifstream file("/proc/meminfo");
  std::string line;
  int values_found(0);
  // Need to set this to zero
  sys_avail = 0;
  while (getline(file, line)) {
    std::istringstream is(line);
    std::string tag;
    long value(0);
    is >> tag >> value;
    if (!is)
      return false;
    if (tag == "MemTotal:") {
      ++values_found;
      sys_total = value;
    } else if (tag == "MemFree:") {
      ++values_found;
      sys_avail += value;
    } else if (tag == "Cached:") {
      ++values_found;
      sys_avail += value;
    } else if (tag == "Buffers:") {
      ++values_found;
      sys_avail += value;
    } else
      continue;
    if (values_found == 4) {
      file.close();
      return true;
    }
  }
  file.close();
  return false;
}
#endif

#ifdef _WIN32
namespace { // Anonymous namespace

MEMORYSTATUSEX
memStatus; ///< A Windows structure holding information about memory usage
}
#endif

/** Attempts to read the system memory statistics.
 * @param sys_avail :: An output variable containing the reported available
 * system memory in this variable in KiB
 * @param sys_total :: An output variable containing the reported total system
 * memory in the system in KiB
 */
void MemoryStats::process_mem_system(size_t &sys_avail, size_t &sys_total) {
  sys_avail = 0;
  sys_total = 0;
#ifdef __linux__
  /*
   * Taken from API/MemoryManager.cpp_LINUX
   *
   * As usual things are more complex on Linux. I think we need to take into
   *account
   * the value of Cached as well since, especially if the system has been
   *running for a long time,
   * MemFree will seem a lot smaller than it should be. To be completely correct
   *we also need to
   * add the value of the "Buffers" as well.
   *
   * The only way I can see as to get acces to the Cached value is from the
   */ proc / meminfo file *so if this is not successful
                I'll fall back to using the sysconf method and *
      forget the cache *
      *RJT(18 / 2 / 10)
      : Should we be using sysinfo() here
      ? * / if (!read_mem_info(sys_avail, sys_total)) {
    long int totPages = sysconf(_SC_PHYS_PAGES);
    long int avPages = sysconf(_SC_AVPHYS_PAGES);
    long int pageSize = sysconf(_SC_PAGESIZE);
    if (totPages < 0)
      totPages = 0;
    if (avPages < 0)
      totPages = 0;
    if (pageSize < 1)
      pageSize = 1;
    // Commented out the next line as the value was being written by the one
    // after
    // sys_avail = avPages / 1024 * pageSize;
    sys_avail = totPages / 1024 * pageSize;
  }
  // Can get the info on the memory that we've already obtained but aren't using
  // right now
  int unusedReserved = mallinfo().fordblks / 1024;
  // unusedReserved can sometimes be negative, which wen added to a low
  // sys_avail will overflow the unsigned int.
  if (unusedReserved < 0)
    unusedReserved = 0;
  // g_log.debug() << "Linux - Adding reserved but unused memory of " <<
  // unusedReserved << " KB\n";
  sys_avail += unusedReserved;

#elif __APPLE__
  // Get the total RAM of the system
  uint64_t totalmem;
  size_t len = sizeof(totalmem);
  // Gives system memory in bytes
  int err = sysctlbyname("hw.memsize", &totalmem, &len, NULL, 0);
  if (err)
    g_log.warning("Unable to obtain memory of system");
  sys_total = totalmem / 1024;

  mach_port_t port = mach_host_self();
  // Need to find out the system page size for next part
  vm_size_t pageSize;
  host_page_size(port, &pageSize);

  // Now get the amount of free memory (=free+inactive memory)
  vm_statistics vmStats;
  mach_msg_type_number_t count;
  count = sizeof(vm_statistics) / sizeof(natural_t);
  err = host_statistics(port, HOST_VM_INFO, (host_info_t)&vmStats, &count);
  if (err)
    g_log.warning("Unable to obtain memory statistics for this Mac.");
  sys_avail = pageSize * (vmStats.free_count + vmStats.inactive_count) / 1024;

  // Now add in reserved but unused memory as reported by malloc
  const size_t unusedReserved = mstats().bytes_free / 1024;
  g_log.debug() << "Mac - Adding reserved but unused memory of "
                << unusedReserved << " KB\n";
  sys_avail += unusedReserved;
#elif _WIN32
  GlobalMemoryStatusEx(&memStatus);
  if (memStatus.ullTotalPhys < memStatus.ullTotalVirtual) {
    sys_avail = static_cast<size_t>(memStatus.ullAvailPhys / 1024);
    sys_total = static_cast<size_t>(memStatus.ullTotalPhys / 1024);
  } else // All virtual memory will be physical, but a process cannot have more
         // than TotalVirtual.
  {
    sys_avail = static_cast<size_t>(memStatus.ullAvailVirtual / 1024);
    sys_total = static_cast<size_t>(memStatus.ullTotalVirtual / 1024);
  }
#endif

  g_log.debug() << "Memory: " << sys_avail << " (free), " << sys_total
                << " (total).\n";
}

/**
 * Initialize platform-dependent options for memory management.
 * On Windows this enables the low-fragmentation heap described here:
 * http://msdn.microsoft.com/en-us/library/aa366750%28v=vs.85%29.aspx
 * On Linux this enables the mmap option for malloc calls to try and release
 * memory more frequently.
 * Note that this function can only be called once
 */
void MemoryOptions::initAllocatorOptions() {
  static bool initialized(false);
  if (initialized)
    return;
#ifdef __linux__
  /* The line below tells malloc to use a different memory allocation system
  * call (mmap) to the 'usual'
  * one (sbrk) for requests above the threshold of the second argument (in
  * bytes). The effect of this
  * is that, for the current threshold value of 8*4096, storage for workspaces
  * having 4096 or greater
  * bins per spectrum will be allocated using mmap.
  * This should have the effect that memory is returned to the kernel as soon as
  * a workspace is deleted,
  * preventing things going to managed workspaces when they shouldn't. This will
  * also hopefully reduce
  * memory fragmentation.
  * Potential downsides to look out for are whether this memory allocation
  * technique makes things
  * noticeably slower and whether it wastes memory (mmap allocates in blocks of
  * the system page size.
  */
  mallopt(M_MMAP_THRESHOLD, 8 * 4096);
#elif _WIN32
  Logger memOptLogger("MemoryOptions");
  // Try to enable the Low Fragmentation Heap for all heaps
  // Bit of a brute force approach, but don't know which heap workspace data
  // ends up on
  HANDLE hHeaps[1025];
  // Get the number of heaps
  const DWORD numHeap = GetProcessHeaps(1024, hHeaps);
  memOptLogger.debug() << "Number of heaps: " << numHeap
                       << "\n"; // GetProcessHeaps(0, NULL) << "\n";
  ULONG ulEnableLFH = 2;        // 2 = Low Fragmentation Heap
  for (DWORD i = 0; i < numHeap; i++) {
    if (!HeapSetInformation(hHeaps[i], HeapCompatibilityInformation,
                            &ulEnableLFH, sizeof(ulEnableLFH))) {
      memOptLogger.debug() << "Failed to enable the LFH for heap " << i << "\n";
    }
  }
#endif
  initialized = true;
}

// ------------------ The actual class ----------------------------------------

/**
 * Constructor
 * @param ignore :: Which memory stats should be ignored.
 */
MemoryStats::MemoryStats(const MemoryStatsIgnore ignore)
    : vm_usage(0), res_usage(0), total_memory(0), avail_memory(0) {

#ifdef _WIN32
  memStatus.dwLength = sizeof(MEMORYSTATUSEX);
#endif

  this->ignoreFields(ignore);
  this->update();
}

/** Update the structure with current information, taking into account what is
 * to be ignored.
 * This call is thread-safe (protected by a mutex).
 * Note: This takes about 0.1 ms on a Ubuntu 10.10 system.
 */
void MemoryStats::update() {
  MemoryStats::mutexMemory.lock();
  // get what is used by the process
  if (this->ignore != MEMORY_STATS_IGNORE_PROCESS) {
    process_mem_usage(this->vm_usage, this->res_usage);
  }

  // get the system information
  if (this->ignore != MEMORY_STATS_IGNORE_SYSTEM) {
    process_mem_system(this->avail_memory, this->total_memory);
  }
  MemoryStats::mutexMemory.unlock();
}

/**
 * Set the fields to ignore
 * @param ignore :: An enumeration giving the fields to ignore
 */
void MemoryStats::ignoreFields(const MemoryStatsIgnore ignore) {
  this->ignore = ignore;
}

/**
 * Returns the virtual memory usage as a string
 * @returns A string containing the amount of virtual memory usage
 */
string MemoryStats::vmUsageStr() const { return memToString(this->vm_usage); }

/**
 * Returns the resident memory used by the current process
 * @returns A string containing the amount of memory the process is using
 */
string MemoryStats::resUsageStr() const { return memToString(this->res_usage); }

/**
 * Returns the total memory of the system as a string
 * @returns A string containing the total amount of memory on the system
 */
string MemoryStats::totalMemStr() const {
  return memToString(this->total_memory);
}

/**
 * Returns the available memory of the system as a string
 * @returns A string containing the amount of available memory on the system
 */
string MemoryStats::availMemStr() const {
  return memToString(this->avail_memory);
}

/**
 * Returns the total memory of the system
 * @returns An unsigned containing the total amount of memory on the system in
 * kiB
 */
size_t MemoryStats::totalMem() const { return this->total_memory; }

/**
 * Returns the available memory of the system in kiB
 * @returns An unsigned containing the available amount of memory on the system
 * in kiB
 */
size_t MemoryStats::availMem() const { return this->avail_memory; }

/**
 * Returns the memory usage of the current process in kiB
 * @returns An unsigned containing the memory used by the current process in kiB
 */
std::size_t MemoryStats::residentMem() const { return this->res_usage; }

/**
 * Returns the virtual memory usage of the current process in kiB
 * @returns An unsigned containing the virtual memory used by the current
 * process in kiB
 */

std::size_t MemoryStats::virtualMem() const { return this->vm_usage; }

/**
 * Returns the reserved memory that has not been factored into the available
 * memory
 * calculation.
 * NOTE: On Windows this can be a lengthy calculation as it involves
 * adding up the reserved space DWORD length at a time. Call only when necessary
 * On other systems this will return 0 as it has already been factored in to the
 * available
 * memory calculation
 * @returns An extra area of memory that can still be allocated.
 */
std::size_t MemoryStats::reservedMem() const {
#ifdef _WIN32
  MEMORY_BASIC_INFORMATION info; // Windows structure
  char *addr = NULL;
  size_t unusedReserved = 0; // total reserved space
  DWORDLONG size = 0;
  GlobalMemoryStatusEx(&memStatus);
  DWORDLONG GB2 =
      memStatus.ullTotalVirtual; // Maximum memory available to the process

  // Loop over all virtual memory to find out the status of every block.
  do {
    VirtualQuery(addr, &info, sizeof(MEMORY_BASIC_INFORMATION));

    // Count up the total size of reserved but unused blocks
    if (info.State == MEM_RESERVE)
      unusedReserved += info.RegionSize;

    addr +=
        info.RegionSize; // Move up to the starting address for the next call
    size += info.RegionSize;
  } while (size < GB2);

  // Convert from bytes to KB
  unusedReserved /= 1024;

  return unusedReserved;
#else
  return 0;
#endif
}

/**
 * The ratio of available to total system memory as a number between 0-100.
 * @returns A percentage
 */
double MemoryStats::getFreeRatio() const {
  return 100. * static_cast<double>(this->avail_memory) /
         static_cast<double>(this->total_memory);
}

/// Convenience function for writting out to stream.
std::ostream &operator<<(std::ostream &out, const MemoryStats &stats) {
  if (stats.ignore != MEMORY_STATS_IGNORE_PROCESS) {
    out << "virtual[" << stats.vmUsageStr() << "] ";
    out << "resident[" << stats.resUsageStr() << "] ";
  }
  if (stats.ignore != MEMORY_STATS_IGNORE_SYSTEM) {
    out << "available[" << stats.availMemStr() << "] ";
    out << "total[" << stats.totalMemStr() << "] ";
  }
  return out;
}

// -------------------------- concrete instantiations
template DLLExport string memToString<uint32_t>(const uint32_t);
template DLLExport string memToString<uint64_t>(const uint64_t);
// To initialize the static class variable.
Mutex MemoryStats::mutexMemory;

} // namespace Kernel
} // namespace Mantid

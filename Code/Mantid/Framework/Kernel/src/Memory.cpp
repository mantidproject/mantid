#include <iostream>
#include <iomanip>
#include <sstream>
#ifdef __linux__
  #include <unistd.h>
  #include <fstream>
  #include<malloc.h>
#endif
#ifdef __APPLE__
  #include <malloc/malloc.h>
  #include <sys/sysctl.h>
  #include <mach/mach_host.h>
#endif

#include "MantidKernel/Memory.h"
#include "MantidKernel/System.h"

using std::size_t;
using std::string;

namespace Mantid
{
namespace Kernel
{

/// Utility function to convert memory in kiB into easy to read units.
template <typename TYPE>
string memToString(const TYPE mem_in_kiB)
{
  std::stringstream buffer;
  if (mem_in_kiB < static_cast<TYPE>(1024))
    buffer << mem_in_kiB << "kiB";
  else if (mem_in_kiB < static_cast<TYPE>(1024 * 1024))
    buffer << (mem_in_kiB/static_cast<TYPE>(1024)) << "MiB";
  else
    buffer << (mem_in_kiB/static_cast<TYPE>(1024*1024)) << "GiB";
  return buffer.str();
}

// -------------------- functions for getting the memory associated with the process

/// Adapted from http://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-run-time-in-c
void process_mem_usage(size_t & vm_usage, size_t & resident_set)
{
  //Temporarily disabled for non-linux OSs
  vm_usage = 0;
  resident_set = 0;

#ifdef __linux__
  using std::ios_base;
  using std::ifstream;

  // 'file' stat seems to give the most reliable results
  ifstream stat_stream("/proc/self/stat",ios_base::in);

  // dummy vars for leading entries in stat that we don't care about
  string pid, comm, state, ppid, pgrp, session, tty_nr;
  string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  string utime, stime, cutime, cstime, priority, nice;
  string O, itrealvalue, starttime;

  // the two fields we want
  unsigned long vsize; // according to man this is %lu
  long rss; // according to man this is %ld

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
              >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
              >> utime >> stime >> cutime >> cstime >> priority >> nice
              >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

  long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
  vm_usage     = static_cast<size_t>(vsize / 1024.0);
  resident_set = static_cast<size_t>(rss * page_size_kb);
#endif
}

// ----------------------- functions associated with getting the memory of the system

#ifdef __linux__
/// This function reads /proc/meminfo to get the system information.
bool read_mem_info(size_t sys_avail, size_t & sys_total)
{
  std::ifstream file("/proc/meminfo");
  std::string line;
  int values_found(0);
  //Need to set this to zero
  sys_avail = 0;
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
      sys_total = value;
    }
    else if (tag == "MemFree:")
    {
      ++values_found;
      sys_avail += value;
    }
    else if (tag == "Cached:")
    {
      ++values_found;
      sys_avail += (8*value/10);
    }
    else
      continue;
    if (values_found == 3)
    {
      file.close();
      return true;
    }
  }
  file.close();
  return false;
}
#endif

void process_mem_system(size_t & sys_avail, size_t & sys_total)
{
  sys_avail = 0;
  sys_total = 0;
#ifdef __linux__
  /*
   * Taken from API/MemoryManager.cpp_LINUX
   *
   * As usual things are more complex on Linux. I think we need to take into account
   * the value of Cached as well since, especially if the system has been running for a long time,
   * MemFree will seem a lot smaller than it should be.
   *
   * The only way I can see as to get acces to the Cached value is from the /proc/meminfo file
   * so if this is not successful I'll fall back to using the sysconf method and forget the cache
   *
   * RJT (18/2/10): Should we be using sysinfo() here?
   */
  if (!read_mem_info(sys_avail, sys_total))
  {
    long int totPages = sysconf(_SC_PHYS_PAGES);
    long int avPages = sysconf(_SC_AVPHYS_PAGES);
    long int pageSize = sysconf(_SC_PAGESIZE);
    sys_avail = avPages / 1024 * pageSize;
    sys_avail = totPages / 1024 * pageSize;
  }
  // Can get the info on the memory that we've already obtained but aren't using right now
  const int unusedReserved = mallinfo().fordblks/1024;
  //g_log.debug() << "Linux - Adding reserved but unused memory of " << unusedReserved << " KB\n"; // TODO uncomment
  sys_avail += unusedReserved;
#elif __APPLE__
  // Get the total RAM of the system
  uint64_t totalmem;
  size_t len = sizeof(totalmem);
  // Gives system memory in bytes
  int err = sysctlbyname("hw.memsize",&totalmem,&len,NULL,0);
  //if (err) g_log.warning("Unable to obtain memory of system");
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
  //if (err) g_log.warning("Unable to obtain memory statistics");
  sys_avail = pageSize * ( vmStats.free_count + vmStats.inactive_count ) / 1024;

  // Now add in reserved but unused memory as reported by malloc
  const size_t unusedReserved = mstats().bytes_free / 1024;
  //g_log.debug() << "Mac - Adding reserved but unused memory of " << unusedReserved << " KB\n";
  sys_avail += unusedReserved;
#endif
  // TODO needs windows stuff folded in
}

// ------------------ The actual class

MemoryStats::MemoryStats(const MemoryStatsIgnore ignore): vm_usage(0), res_usage(0),
    total_memory(0), avail_memory(0)
{
  this->ignoreFields(ignore);
  this->update();
}

void MemoryStats::update()
{
  // get what is used by the process
  if (this->ignore != MEMORY_STATS_IGNORE_PROCESS)
    process_mem_usage(this->vm_usage, this->res_usage);

  // get the system information
  if (this->ignore != MEMORY_STATS_IGNORE_SYSTEM)
  process_mem_system(this->avail_memory, this->total_memory);
}

void MemoryStats::ignoreFields(const MemoryStatsIgnore ignore) {
  this->ignore = ignore;
}

string MemoryStats::vmUsageStr() const
{
  return memToString(this->vm_usage);
}

string MemoryStats::resUsageStr() const
{
  return memToString(this->res_usage);
}

string MemoryStats::totalMemStr() const
{
  return memToString(this->total_memory);
}

string MemoryStats::availMemStr() const
{
  return memToString(this->avail_memory);
}

size_t MemoryStats::totalMem() const
{
  return this->total_memory;
}

size_t MemoryStats::availMem() const
{
  return this->avail_memory;
}

/// The ration of available to total system memory as a number between 0-100.
double MemoryStats::getFreeRatio() const
{
  return 100. * static_cast<double>(this->avail_memory) / static_cast<double>(this->total_memory);
}

/// Convenience function for writting out to stream.
std::ostream& operator<<(std::ostream& out, const MemoryStats &stats)
{
  if ( stats.ignore != MEMORY_STATS_IGNORE_PROCESS) {
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

} // namespace Kernel
} // namespace Mantid

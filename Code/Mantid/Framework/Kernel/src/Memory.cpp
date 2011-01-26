#include <iostream>
#include <iomanip>
#include <sstream>
#if defined __linux__
  #include <unistd.h>
  #include <fstream>
#endif

#include "MantidKernel/Memory.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

template <typename TYPE>
std::string memToString(const TYPE mem_in_kiB)
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

void process_mem_usage(std::size_t & vm_usage, std::size_t & resident_set)
{
#ifdef __linux__
  using std::ios_base;
  using std::ifstream;
  using std::string;

  vm_usage     = 0.0;
  resident_set = 0.0;

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
  vm_usage     = static_cast<std::size_t>(vsize / 1024.0);
  resident_set = static_cast<std::size_t>(rss * page_size_kb);
#else _WIN32
  //Temporarily disabled for non-linux OSs
  vm_usage = 0;
  resident_set = 0;
#endif
}

// -------------------------- concrete instantiations
template DLLExport std::string memToString<uint32_t>(const uint32_t);
template DLLExport std::string memToString<uint64_t>(const uint64_t);

} // namespace Kernel
} // namespace Mantid

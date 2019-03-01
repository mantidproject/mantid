#include "MantidAPI/AlgoTimeRegister.h"
#include "MantidKernel/MultiThreaded.h"
#include <fstream>
#include <time.h>

namespace Mantid {
namespace Instrumentation {

AlgoTimeRegister::Dump::Dump(AlgoTimeRegister &atr, const std::string &nm)
    : m_algoTimeRegister(atr), m_name(nm) {
  clock_gettime(CLOCK_MONOTONIC, &m_regStart);
}

timespec AlgoTimeRegister::diff(timespec start, timespec end) {
  timespec temp;
  if ((end.tv_nsec - start.tv_nsec) < 0) {
    temp.tv_sec = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return temp;
}

AlgoTimeRegister::Dump::~Dump() {
  timespec regFinish;
  clock_gettime(CLOCK_MONOTONIC, &regFinish);
  {
    std::lock_guard<std::mutex> lock(m_algoTimeRegister.m_mutex);
    m_algoTimeRegister.m_info.emplace_back(m_name, std::this_thread::get_id(),
                                           m_regStart, regFinish);
  }
}

AlgoTimeRegister::AlgoTimeRegister()
    : m_start(std::chrono::high_resolution_clock::now()) {
  clock_gettime(CLOCK_MONOTONIC, &m_hstart);
}

AlgoTimeRegister::~AlgoTimeRegister() {
  std::fstream fs;
  fs.open("./algotimeregister.out", std::ios::out);
  fs << "START_POINT: "
     << std::chrono::duration_cast<std::chrono::nanoseconds>(
            m_start.time_since_epoch())
            .count()
     << " MAX_THREAD: " << PARALLEL_GET_MAX_THREADS << "\n";
  for (auto &elem : m_info) {
    auto st = diff(m_hstart, elem.m_begin);
    auto fi = diff(m_hstart, elem.m_end);
    fs << "ThreadID=" << elem.m_threadId << ", AlgorithmName=" << elem.m_name
       << ", StartTime=" << std::size_t(st.tv_sec * 1000000000) + st.tv_nsec
       << ", EndTime=" << std::size_t(fi.tv_sec * 1000000000) + fi.tv_nsec
       << "\n";
  }
}

} // namespace Instrumentation
} // namespace Mantid

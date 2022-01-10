// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgoTimeRegister.h"
#include "MantidKernel/MultiThreaded.h"
#include <fstream>
#include <time.h>

namespace Mantid {
namespace Instrumentation {

using Kernel::time_point_ns;

AlgoTimeRegister::Dump::Dump(AlgoTimeRegister &atr, const std::string &nm)
    : m_algoTimeRegister(atr), m_regStart_chrono(std::chrono::high_resolution_clock::now()), m_name(nm) {}

AlgoTimeRegister::Dump::~Dump() {
  const time_point_ns regFinish = std::chrono::high_resolution_clock::now();
  {
    std::lock_guard<std::mutex> lock(m_algoTimeRegister.m_mutex);
    m_algoTimeRegister.addTime(m_name, std::this_thread::get_id(), m_regStart_chrono, regFinish);
  }
}

void AlgoTimeRegister::addTime(const std::string &name, const std::thread::id thread_id,
                               const Kernel::time_point_ns &begin, const Kernel::time_point_ns &end) {
  m_info.emplace_back(name, thread_id, begin, end);
}

void AlgoTimeRegister::addTime(const std::string &name, const Kernel::time_point_ns &begin,
                               const Kernel::time_point_ns &end) {
  this->addTime(name, std::this_thread::get_id(), begin, end);
}

AlgoTimeRegister::AlgoTimeRegister() : m_start(std::chrono::high_resolution_clock::now()) {}

AlgoTimeRegister::~AlgoTimeRegister() {
  std::fstream fs;
  fs.open("./algotimeregister.out", std::ios::out);
  // c++20 has an implementation of operator<<
  fs << "START_POINT: " << std::chrono::duration_cast<std::chrono::nanoseconds>(m_start.time_since_epoch()).count()
     << " MAX_THREAD: " << PARALLEL_GET_MAX_THREADS << "\n";
  for (auto &elem : m_info) {
    const std::chrono::nanoseconds st = elem.m_begin - m_start;
    const std::chrono::nanoseconds fi = elem.m_end - m_start;
    fs << "ThreadID=" << elem.m_threadId << ", AlgorithmName=" << elem.m_name << ", StartTime=" << st.count()
       << ", EndTime=" << fi.count() << "\n";
  }
}

} // namespace Instrumentation
} // namespace Mantid

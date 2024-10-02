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

using Kernel::ConfigService;
using Kernel::time_point_ns;

AlgoTimeRegisterImpl::Dump::Dump(const std::string &nm)
    : m_regStart_chrono(std::chrono::high_resolution_clock::now()), m_name(nm) {}

AlgoTimeRegisterImpl::Dump::~Dump() {
  const time_point_ns regFinish = std::chrono::high_resolution_clock::now();
  {
    // auto atr = ;
    std::lock_guard<std::mutex> lock(AlgoTimeRegister::Instance().m_mutex);
    AlgoTimeRegister::Instance().addTime(m_name, std::this_thread::get_id(), m_regStart_chrono, regFinish);
  }
}

void AlgoTimeRegisterImpl::addTime(const std::string &name, const std::thread::id thread_id,
                                   const Kernel::time_point_ns &begin, const Kernel::time_point_ns &end) {
  m_info.emplace_back(name, thread_id, begin, end);
}

void AlgoTimeRegisterImpl::addTime(const std::string &name, const Kernel::time_point_ns &begin,
                                   const Kernel::time_point_ns &end) {
  this->addTime(name, std::this_thread::get_id(), begin, end);
}

void AlgoTimeRegisterImpl::test(const int a, const int b) { printf("a + b = %d\n", a + b); }

AlgoTimeRegisterImpl::AlgoTimeRegisterImpl() : m_start(std::chrono::high_resolution_clock::now()) {}

AlgoTimeRegisterImpl::~AlgoTimeRegisterImpl() {
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

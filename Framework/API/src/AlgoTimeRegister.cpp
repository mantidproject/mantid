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

namespace {
Kernel::Logger &LOGGER() {
  static Kernel::Logger logger("AlgoTimeRegister");
  return logger;
}
} // namespace

using Kernel::ConfigService;
using Kernel::time_point_ns;

AlgoTimeRegisterImpl::Dump::Dump(const std::string &nm)
    : m_regStart_chrono(std::chrono::high_resolution_clock::now()), m_name(nm) {}

AlgoTimeRegisterImpl::Dump::~Dump() {
  const time_point_ns regFinish = std::chrono::high_resolution_clock::now();
  { AlgoTimeRegister::Instance().addTime(m_name, std::this_thread::get_id(), m_regStart_chrono, regFinish); }
}

void AlgoTimeRegisterImpl::addTime(const std::string &name, const Kernel::time_point_ns &begin,
                                   const Kernel::time_point_ns &end) {
  AlgoTimeRegister::Instance().addTime(name, std::this_thread::get_id(), begin, end);
}

bool AlgoTimeRegisterImpl::writeToFile() {
  auto writeEnable = Kernel::ConfigService::Instance().getValue<bool>("performancelog.write").value();
  if (!writeEnable) {
    LOGGER().debug() << "performancelog.write is disabled (off/0/false)\n";
    return false;
  }
  auto filename = Kernel::ConfigService::Instance().getString("performancelog.filename");
  if (filename.empty()) {
    LOGGER().debug() << "performancelog.filename is empty, please provide valid filename\n";
    return false;
  }
  if (m_filename == filename && m_hasWrittenToFile) {
    return true;
  }

  m_filename = filename;

  LOGGER().debug() << "Performance log file: " << m_filename << '\n';

  std::fstream fs;

  fs.open(m_filename, std::ios::out);
  if (fs.is_open()) {
    fs << "START_POINT: " << std::chrono::duration_cast<std::chrono::nanoseconds>(m_start.time_since_epoch()).count()
       << " MAX_THREAD: " << PARALLEL_GET_MAX_THREADS << "\n";
    fs.close();
    m_hasWrittenToFile = true;
  } else {
    LOGGER().notice() << "Failed to open the file, timing will not write to file.\n";
    return false;
  }
  return true;
}

void AlgoTimeRegisterImpl::addTime(const std::string &name, const std::thread::id thread_id,
                                   const Kernel::time_point_ns &begin, const Kernel::time_point_ns &end) {
  std::lock_guard<std::mutex> lock(AlgoTimeRegister::Instance().m_mutex);
  if (writeToFile()) {
    std::fstream fs;
    fs.open(m_filename, std::ios::out | std::ios::app);
    if (fs.is_open()) {
      const std::chrono::nanoseconds st = begin - m_start;
      const std::chrono::nanoseconds fi = end - m_start;
      fs << "ThreadID=" << thread_id << ", AlgorithmName=" << name << ", StartTime=" << st.count()
         << ", EndTime=" << fi.count() << "\n";

      fs.close();
    }
  }
}

AlgoTimeRegisterImpl::AlgoTimeRegisterImpl()
    : m_start(std::chrono::high_resolution_clock::now()), m_hasWrittenToFile(false) {}

AlgoTimeRegisterImpl::~AlgoTimeRegisterImpl() {}

} // namespace Instrumentation
} // namespace Mantid
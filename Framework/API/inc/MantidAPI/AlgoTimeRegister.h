// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <mutex>
#include <thread>
#include <vector>

#include "MantidAPI/DllConfig.h"

class timespec;

namespace Mantid {
namespace Instrumentation {

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_ns;

/** AlgoTimeRegister : simple class to dump information about executed
 * algorithms
 */
class AlgoTimeRegister {
public:
  static AlgoTimeRegister globalAlgoTimeRegister;
  static timespec diff(timespec start, timespec end);
  struct Info {
    std::string m_name;
    std::thread::id m_threadId;
    time_point_ns m_begin;
    time_point_ns m_end;

    Info(const std::string &nm, const std::thread::id &id, const time_point_ns &be, const time_point_ns &en)
        : m_name(nm), m_threadId(id), m_begin(be), m_end(en) {}
  };

  class Dump {
    AlgoTimeRegister &m_algoTimeRegister;
    timespec m_regStart;
    time_point_ns m_regStart_chrono;

    const std::string m_name;

  public:
    Dump(AlgoTimeRegister &atr, const std::string &nm);
    ~Dump();
  };

  void addTime(const std::string &name, const std::thread::id thread_id, const time_point_ns &begin,
               const time_point_ns &end);
  void addTime(const std::string &name, const time_point_ns &begin, const time_point_ns &end);
  AlgoTimeRegister();
  ~AlgoTimeRegister();

private:
  std::mutex m_mutex;
  std::vector<Info> m_info;
  time_point_ns m_start;
};

} // namespace Instrumentation
} // namespace Mantid

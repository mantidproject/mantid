// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ALGOTIMEREGISTER_H_
#define MANTID_API_ALGOTIMEREGISTER_H_

#include <mutex>
#include <thread>
#include <vector>

#include "MantidAPI/DllConfig.h"

class timespec;

namespace Mantid {
namespace Instrumentation {

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
    timespec m_begin;
    timespec m_end;

    Info(const std::string &nm, const std::thread::id &id, const timespec &be,
         const timespec &en)
        : m_name(nm), m_threadId(id), m_begin(be), m_end(en) {}
  };

  class Dump {
    AlgoTimeRegister &m_algoTimeRegister;
    timespec m_regStart;
    const std::string m_name;

  public:
    Dump(AlgoTimeRegister &atr, const std::string &nm);
    ~Dump();
  };

  AlgoTimeRegister();
  ~AlgoTimeRegister();

private:
  std::mutex m_mutex;
  std::vector<Info> m_info;
  timespec m_hstart;
  std::chrono::high_resolution_clock::time_point m_start;
};

} // namespace Instrumentation
} // namespace Mantid

#endif /* MANTID_API_ALGOTIMEREGISTER_H_ */
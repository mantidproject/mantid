// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Timer.h"
#include <mutex>
#include <thread>
#include <vector>

#include "MantidAPI/DllConfig.h"

using Mantid::Kernel::Logger;

namespace Mantid {
namespace Instrumentation {

/** AlgoTimeRegister : simple class to dump information about executed
 * algorithms
 */
class MANTID_API_DLL AlgoTimeRegisterImpl {
public:
  AlgoTimeRegisterImpl(const AlgoTimeRegisterImpl &) = delete;
  AlgoTimeRegisterImpl &operator=(const AlgoTimeRegisterImpl &) = delete;

  struct Info {
    std::string m_name;
    std::thread::id m_threadId;
    Kernel::time_point_ns m_begin;
    Kernel::time_point_ns m_end;

    Info(const std::string &nm, const std::thread::id &id, const Kernel::time_point_ns &be,
         const Kernel::time_point_ns &en)
        : m_name(nm), m_threadId(id), m_begin(be), m_end(en) {}
  };

  class Dump {
    Kernel::time_point_ns m_regStart_chrono;
    const std::string m_name;

  public:
    Dump(const std::string &nm);
    ~Dump();
  };

  void addTime(const std::string &name, const std::thread::id thread_id, const Kernel::time_point_ns &begin,
               const Kernel::time_point_ns &end);
  void addTime(const std::string &name, const Kernel::time_point_ns &begin, const Kernel::time_point_ns &end);
  void test(const int a, const int b);
  std::mutex m_mutex;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgoTimeRegisterImpl>;

  AlgoTimeRegisterImpl();
  ~AlgoTimeRegisterImpl();

  static Logger &g_log;
  std::vector<Info> m_info;
  Kernel::time_point_ns m_start;
};

using AlgoTimeRegister = Mantid::Kernel::SingletonHolder<AlgoTimeRegisterImpl>;

} // namespace Instrumentation
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::Instrumentation::AlgoTimeRegisterImpl>;
} // namespace Kernel
} // namespace Mantid

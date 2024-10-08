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

namespace Mantid {
namespace Instrumentation {

/** AlgoTimeRegister : simple class to dump information about executed
 * algorithms
 */
class MANTID_API_DLL AlgoTimeRegisterImpl {
public:
  AlgoTimeRegisterImpl(const AlgoTimeRegisterImpl &) = delete;
  AlgoTimeRegisterImpl &operator=(const AlgoTimeRegisterImpl &) = delete;

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

  std::mutex m_mutex;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgoTimeRegisterImpl>;

  AlgoTimeRegisterImpl();
  ~AlgoTimeRegisterImpl();

  bool writeToFile();

  Kernel::time_point_ns m_start;
  std::string m_filename;
  bool m_hasWrittenToFile;
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

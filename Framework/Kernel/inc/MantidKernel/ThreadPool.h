// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ThreadScheduler.h"
#include <vector>

// Forward declares

namespace Poco {
class Thread;
}

namespace Mantid {
namespace Kernel {
class ProgressBase;
class Task;
class ThreadPoolRunnable;

/** A Thread Pool implementation that keeps a certain number of
 * threads running (normally, equal to the number of hardware cores available)
 * and schedules tasks to them in the most efficient way possible.
 *
 * This implementation will be slanted towards performing many more
 * Task's than there are available cores, so threads will be reused.

  @author Janik Zikovsky, SNS
  @date Feb 7, 2011
 */

class MANTID_KERNEL_DLL ThreadPool final {
public:
  ThreadPool(ThreadScheduler *scheduler = new ThreadSchedulerFIFO(), size_t numThreads = 0,
             ProgressBase *prog = nullptr);

  ~ThreadPool();

  void start(double waitSec = 0.0);

  void schedule(const std::shared_ptr<Task> &task, bool start = false);

  void joinAll();

  static size_t getNumPhysicalCores();

protected:
  /// Number of cores used
  size_t m_numThreads;

  /// The ThreadScheduler instance taking care of task scheduling
  std::unique_ptr<ThreadScheduler> m_scheduler;

  /// Vector with all the threads that are started
  std::vector<std::unique_ptr<Poco::Thread>> m_threads;

  /// Vector of the POCO-required classes to actually run in a thread.
  std::vector<std::unique_ptr<ThreadPoolRunnable>> m_runnables;

  /// Have the threads started?
  bool m_started;

  /// Progress reporter
  std::unique_ptr<ProgressBase> m_prog;

private:
  // prohibit default copy constructor as it does not work
  ThreadPool(const ThreadPool &);
  // prohibit asighnment as it does not work
  ThreadPool &operator=(const ThreadPool &other);
};

///// Singleton declaration
// #ifdef _WIN32
//     // this breaks new namespace declaraion rules; need to find a better fix
//     template class MANTID_KERNEL_DLL
//     Mantid::Kernel::SingletonHolder<ThreadPoolImpl>;
// #endif /* _WIN32 */
//     typedef Mantid::Kernel::SingletonHolder<ThreadPoolImpl>
//     ThreadPool;

} // namespace Kernel
} // namespace Mantid

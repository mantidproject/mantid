#ifndef MANTID_KERNEL_THREADPOOLRUNNABLE_H_
#define MANTID_KERNEL_THREADPOOLRUNNABLE_H_

#include "MantidKernel/DllConfig.h"
#include <Poco/Runnable.h>

namespace Mantid {
namespace Kernel {
// Forward declares
class ProgressBase;
class ThreadScheduler;

/** ThreadPoolRunnable : Class used by thread pool (and POCO) to
 * run inside a thread.
 *
 * @author Janik Zikovsky
 * @date 2011-02-23
 */
class MANTID_KERNEL_DLL ThreadPoolRunnable : public Poco::Runnable {
public:
  ThreadPoolRunnable(size_t threadnum, ThreadScheduler *scheduler,
                     ProgressBase *prog = nullptr, double waitSec = 0.0);

  /// Return the thread number of this thread.
  size_t threadnum() { return m_threadnum; }

  void run() override;

  void clearWait();

private:
  /// ID of this thread.
  size_t m_threadnum;

  /// The ThreadScheduler instance taking care of task scheduling
  ThreadScheduler *m_scheduler;

  /// Progress reporter
  ProgressBase *m_prog;

  /// How many seconds you are allowed to wait with no tasks before exiting.
  double m_waitSec;
};

} // namespace Mantid
} // namespace Kernel

#endif /* MANTID_KERNEL_THREADPOOLRUNNABLE_H_ */

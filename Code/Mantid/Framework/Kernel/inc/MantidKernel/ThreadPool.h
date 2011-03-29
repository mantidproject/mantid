#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/ProgressBase.h"
#include <vector>
#include <Poco/Thread.h>

namespace Mantid
{

namespace Kernel
{

  /** A Thread Pool implementation that keeps a certain number of
   * threads running (normally, equal to the number of hardware cores available)
   * and schedules tasks to them in the most efficient way possible.
   *
   * This implementation will be slanted towards performing many more
   * Task's than there are available cores, so threads will be reused.

    @author Janik Zikovsky, SNS
    @date Feb 7, 2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
   */

  class DLLExport ThreadPool
  {
  public:
    ThreadPool(ThreadScheduler * scheduler = new ThreadSchedulerFIFO(), size_t numCores = 0,
        ProgressBase * prog = NULL);

    ~ThreadPool();

    void start();

    void schedule(Task * task, bool start = false);

    void joinAll();

    static size_t getNumPhysicalCores();


  protected:
    /// Number of cores used
    size_t m_numThreads;

    /// The ThreadScheduler instance taking care of task scheduling
    ThreadScheduler * m_scheduler;

    /// Vector with all the threads that are started
    std::vector<Poco::Thread *> m_threads;

    /// Vector of the POCO-required classes to actually run in a thread.
    std::vector<ThreadPoolRunnable *> m_runnables;

    /// Have the threads started?
    bool m_started;

    /// Progress reporter
    ProgressBase * m_prog;
  };





///// Singleton declaration
//#ifdef _WIN32
//    // this breaks new namespace declaraion rules; need to find a better fix
//    template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ThreadPoolImpl>;
//#endif /* _WIN32 */
//    typedef EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ThreadPoolImpl> ThreadPool;


} // namespace Kernel
} // namespace Mantid

#endif /* THREADPOOL_H_ */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include <vector>

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
    ThreadPool(size_t numCores = 0);

    void schedule(Task * task);

    void run(bool sort = false);


  protected:
    /// Number of cores used
    size_t m_numThreads;

    /// List of scheduled tasks
    std::vector<Task *> m_tasks;
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

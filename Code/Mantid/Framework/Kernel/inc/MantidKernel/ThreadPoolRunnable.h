#ifndef MANTID_KERNEL_THREADPOOLRUNNABLE_H_
#define MANTID_KERNEL_THREADPOOLRUNNABLE_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadScheduler.h"
#include <Poco/Mutex.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>


namespace Mantid
{
namespace Kernel
{

  /** ThreadPoolRunnable : Class used by thread pool (and POCO) to
   * run inside a thread.
   * 
   * @author: Janik Zikovsky
   * @date: 2011-02-23 10:40:46.297133
   */
  class ThreadPoolRunnable: public Poco::Runnable
  {
  public:
    ThreadPoolRunnable(size_t threadnum, ThreadScheduler * scheduler);
    ~ThreadPoolRunnable();

    /// Return the thread number of this thread.
    size_t threadnum()
    { return m_threadnum; }

    virtual void run();


  private:
    /// ID of this thread.
    size_t m_threadnum;

    /// The ThreadScheduler instance taking care of task scheduling
    ThreadScheduler * m_scheduler;
  };


} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_THREADPOOLRUNNABLE_H_ */

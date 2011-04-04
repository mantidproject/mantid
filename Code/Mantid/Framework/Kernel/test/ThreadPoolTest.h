#ifndef THREADPOOLTEST_H_
#define THREADPOOLTEST_H_

#include <cxxtest/TestSuite.h>

#include <MantidKernel/Timer.h>
#include <MantidKernel/FunctionTask.h>
#include <MantidKernel/ProgressText.h>
#include "MantidKernel/MultiThreaded.h"
#include <MantidKernel/ThreadPool.h>
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"

#include <boost/bind.hpp>
#include <iostream>
#include <iomanip>
#include <Poco/Mutex.h>
#include <cstdlib>

using namespace Mantid::Kernel;

//#include <boost/thread.hpp>

//=======================================================================================

class TimeWaster
{
public:
static size_t waste_time(double seconds)
{
  // Waste time, but use up the CPU!
  std::size_t num = 0;
  Mantid::Kernel::Timer time;
  while (time.elapsed(false) < seconds)
  {
    double x = 1.1;
    for (int j=0; j < 100000; j++)
    {
      x = x * x;
      x = x + x;
      x = x / 1.1;
    }
    num += 1;
  }
  return num;
}


void waste_time_with_lock(double seconds)
{
  {
    Mutex::ScopedLock lock(m_mutex);
    std::cout << "waste_time for " << seconds << " seconds." << std::endl;
  }
  waste_time(seconds);
}


/** Add a number but use a lock to avoid contention */
void add_to_number(size_t adding)
{
  {
    Mutex::ScopedLock lock(m_mutex);
    total += adding;
  }
}

private:
  Mutex m_mutex;

public:
  size_t total;
};


//=======================================================================================

int threadpooltest_check = 0;

void threadpooltest_function()
{
  threadpooltest_check = 12;
}


std::vector<int> threadpooltest_vec;
void threadpooltest_adding_stuff(int val)
{
  //TODO: Mutex
  threadpooltest_vec.push_back(val);
}



// Counter for the test.
size_t TaskThatAddsTasks_counter;
Mutex TaskThatAddsTasks_mutex;

//=======================================================================================
/** Class that adds tasks to its scheduler */
class TaskThatAddsTasks : public Task
{
public:
  // ctor
  TaskThatAddsTasks(ThreadScheduler * scheduler, size_t depth)
  : Task(), m_scheduler(scheduler), depth(depth)
  {
    // Use a randomized cost function; this will have an effect on the sorted schedulers.
    m_cost = rand();
  }

  // Run the task
  void run()
  {
    if (depth < 4)
    {
      // Add ten tasks (one level deeper)
      for (size_t i=0; i<10; i++)
      {
        m_scheduler->push(new TaskThatAddsTasks(m_scheduler, depth+1));
      }
    }
    else
    {
      // Lock to ensure you don't step on yourself.
      Mutex::ScopedLock lock(TaskThatAddsTasks_mutex);
      // Increment the counter only at the lowest level.
      TaskThatAddsTasks_counter += 1;
    }
  }

private:
  ThreadScheduler * m_scheduler;
  size_t depth;
};



int ThreadPoolTest_TaskThatThrows_counter = 0;

//=======================================================================================
class ThreadPoolTest : public CxxTest::TestSuite
{
public:

  /** Test that shows that OPENMP does not use a thread pool idea to optimally allocate threads
   * (unless you use schedule(dynamic) )! */
  void xtestOpenMP()
  {
    Timer overall;
    int num = 16;
    //PARALLEL_FOR_NO_WSP_CHECK()
//#pragma omp parallel for schedule(dynamic)
    for (int i=0; i<num; i++)
    {
      double delay = num-i;
      PARALLEL_CRITICAL(test1)
      std::cout << std::setw(5) << i << ": Thread " << PARALLEL_THREAD_NUMBER << " will delay for " << delay << " seconds." << std::endl;
      TimeWaster::waste_time(delay);
      PARALLEL_CRITICAL(test1)
      std::cout << std::setw(5) << i << ": is done." << std::endl;
    }
    std::cout << overall.elapsed() << " secs total.\n";
  }

  /** Make it waste time, 0 to 16 seconds
   * DISABLED because it is (intentionally) slow. */
  void xtest_Scheduler_LargestCostFirst_wastetime()
  {
    ThreadPool p(new ThreadSchedulerFIFO(), 0);
    threadpooltest_vec.clear();
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 0);
    TimeWaster mywaster;

    for (int i=0; i< 16; i++)
    {
      double cost = i; // time is exactly i
      // Bind to a member function of mywaster
      p.schedule( new FunctionTask( boost::bind(&TimeWaster::waste_time_with_lock, &mywaster, i), cost ) );
    }

    Timer overall;

    TS_ASSERT_THROWS_NOTHING( p.joinAll() );

    std::cout << overall.elapsed() << " secs total." << std::endl;
  }

  /** Speed comparison of test
   * DISABLED: because it is not necessary
   */
  void xtest_compare()
  {
    size_t total=0;
    ThreadScheduler * sched = new ThreadSchedulerLargestCost();
    for (size_t i=0; i<100000; i++)
    {
      total += 1;
      sched->push(new FunctionTask( boost::bind(TimeWaster::waste_time, i*1.0), i*1.0 ));
    }
    size_t other = total;
    //std::cout << total << std::endl;
  }

//
//  void xtest_Boost_single_threads()
//  {
//    Mantid::Kernel::Timer overall;
//    double time;
//    size_t num = 10000;
//
//    for (size_t i=0; i < num; i++)
//    {
//      DoNothingBoost myDoNothing;
//      boost::thread workerThread(myDoNothing);
//      workerThread.join();
//    }
//    time = overall.elapsed();
//    std::cout << "Boost: " <<std::setw(15) << time << " secs total = " << std::setw(15) << (num*1.0/time) << " per second" << std::endl;
//  }


  void test_Constructor()
  {
    ThreadPool p;
  }

  void test_schedule()
  {
    ThreadPool p;
    TS_ASSERT_EQUALS( threadpooltest_check, 0);
    p.schedule( new FunctionTask( threadpooltest_function ) );
    TS_ASSERT_EQUALS( threadpooltest_check, 0);
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    TS_ASSERT_EQUALS( threadpooltest_check, 12);
  }



  //=======================================================================================
  //=======================================================================================
  /** Class for debugging progress reporting */
  class MyTestProgress : public ProgressBase
  {
  public:
    MyTestProgress(double start,double end, int numSteps, ThreadPoolTest * myParent)
    : ProgressBase(start,end, numSteps), parent(myParent)
    {
    }

    void doReport(const std::string& msg = "")
    {
      parent->last_report_message = msg;
      parent->last_report_counter = m_i;
      double p = m_start + m_step*(m_i - m_ifirst);
      parent->last_report_value = p;
    }

  public:
    ThreadPoolTest * parent;
  };

  /// Index that was last set at doReport
  int last_report_counter;
  double last_report_value;
  std::string last_report_message;

  void test_with_progress_reporting()
  {
    last_report_counter = 0;
    ThreadPool p(new ThreadSchedulerFIFO(), 1, new MyTestProgress(0.0, 1.0, 10, this));
    for (int i=0; i< 10; i++)
    {
      double cost = i;
      p.schedule( new FunctionTask( threadpooltest_function, cost ) );
    }
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    // The test reporter was called
    TS_ASSERT_EQUALS( last_report_counter, 10);
  }

  /// Disabled because it has std output
  void xtest_with_progress_reporting2()
  {
    ThreadPool p(new ThreadSchedulerFIFO(), 0, new ProgressText(0.0, 1.0, 50));
    for (int i=0; i< 50; i++)
    {
      double cost = i;
      p.schedule( new FunctionTask( threadpooltest_function, cost ) );
    }
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
  }



  //=======================================================================================

  /** Start a threadpool before adding tasks
   * DISABLED because the timing issues make it somewhat unreliable under heavy loads. */
  void xtest_start_and_wait()
  {
    ThreadPool p; // Makes a default scheduler
    threadpooltest_check = 0;

    // Start and allow it to wait for 1 second
    p.start(1.0);

    // Simulate doing some work
    Poco::Thread::sleep(40);

    // Now you add the task
    p.schedule( new FunctionTask( threadpooltest_function ) );

    // Simulate doing more work (this allows the task to run)
    Poco::Thread::sleep(40);

    // The task ran before we called joinAll(). Magic!
    TS_ASSERT_EQUALS( threadpooltest_check, 12);

    // Reset and try again. The threads are still waiting, it has been less than 1 second.
    threadpooltest_check = 0;
    p.schedule( new FunctionTask( threadpooltest_function ) );
    Poco::Thread::sleep(40);
    TS_ASSERT_EQUALS( threadpooltest_check, 12);

    // You still need to call joinAll() to clean up everything.
    p.joinAll();

    // Ok, the task did execute.
    TS_ASSERT_EQUALS( threadpooltest_check, 12);
  }

  /** Start a threadpool before adding tasks. But the wait time was too short!
   * DISABLED because the timing issues make it somewhat unreliable under heavy loads. */
  void xtest_start_and_wait_short_wait_time()
  {
    ThreadPool p; // Makes a default scheduler
    threadpooltest_check = 0;

    // Start and allow it to wait for a very short time
    p.start(0.03);

    // But it takes too long before the task is actually added
    Poco::Thread::sleep(100);
    p.schedule( new FunctionTask( threadpooltest_function ) );
    Poco::Thread::sleep(30);
    // So the task has not run, since the threads exited before!
    TS_ASSERT_EQUALS( threadpooltest_check, 0);

    // But you can still call joinAll() to run the task that is waiting.
    p.joinAll();
    // Ok, the task did execute.
    TS_ASSERT_EQUALS( threadpooltest_check, 12);
  }


  //=======================================================================================

  /** We schedule a task, run the threads, but don't abort them.
   * Then we re-schedule stuff, and re-join.
   */
  void test_schedule_resume_tasks()
  {
    ThreadPool p; // Makes a default scheduler
    threadpooltest_check = 0;
    p.schedule( new FunctionTask( threadpooltest_function ) );
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    // Ok, the task did execute.
    TS_ASSERT_EQUALS( threadpooltest_check, 12);

    // Now we reset.
    threadpooltest_check = 0;
    p.schedule( new FunctionTask( threadpooltest_function ) );
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    TS_ASSERT_EQUALS( threadpooltest_check, 12);
  }

  void test_Scheduler_FIFO()
  {
    // Only use one core, it'll make things simpler
    ThreadPool p(new ThreadSchedulerFIFO(), 1);

    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 0);
    for (int i=0; i< 10; i++)
    {
      double cost = i;
      p.schedule( new FunctionTask( boost::bind(threadpooltest_adding_stuff, i), cost ) );
    }
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 10);
    if (threadpooltest_vec.size() < 10) return;
    // The first ones added are the first ones run.
    TS_ASSERT_EQUALS( threadpooltest_vec[0], 0);
    TS_ASSERT_EQUALS( threadpooltest_vec[1], 1);
    TS_ASSERT_EQUALS( threadpooltest_vec[2], 2);
  }


  void test_Scheduler_LIFO()
  {
    ThreadPool p(new ThreadSchedulerLIFO(), 1);
    threadpooltest_vec.clear();
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 0);
    for (int i=0; i< 10; i++)
    {
      double cost = i;
      p.schedule( new FunctionTask( boost::bind(threadpooltest_adding_stuff, i), cost ) );
    }
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 10);
    // The last ones added are the first ones run.
    TS_ASSERT_EQUALS( threadpooltest_vec[0], 9);
    TS_ASSERT_EQUALS( threadpooltest_vec[1], 8);
    TS_ASSERT_EQUALS( threadpooltest_vec[2], 7);
  }

  void test_Scheduler_LargestCostFirst()
  {
    // Only use one core, it'll make things simpler
    ThreadPool p(new ThreadSchedulerLargestCost(), 1);
    threadpooltest_vec.clear();
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 0);
    for (int i=0; i< 10; i++)
    {
      double cost = i;
      p.schedule( new FunctionTask( boost::bind(threadpooltest_adding_stuff, i), cost ) );
    }
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    TS_ASSERT_EQUALS( threadpooltest_vec.size(), 10);
    // The first ones added are the first ones run.
    TS_ASSERT_EQUALS( threadpooltest_vec[0], 9);
    TS_ASSERT_EQUALS( threadpooltest_vec[1], 8);
    TS_ASSERT_EQUALS( threadpooltest_vec[2], 7);
  }



  //--------------------------------------------------------------------
  /** Perform a stress test on the given scheduler.
   * This runs a large number of super-short tasks; enough that the
   * queue locking is tested against simultaneous access. A segfault
   * results if the queue is improperly accessed.
   */
  void do_StressTest_scheduler(ThreadScheduler * sched)
  {
    ThreadPool p(sched, 0);
    TimeWaster mywaster;
    size_t num = 30000;
    mywaster.total = 0;
    Mutex * lastMutex = NULL;
    for (size_t i=0; i<=num; i++)
    {
      Task * task = new FunctionTask( boost::bind(&TimeWaster::add_to_number, &mywaster, i), i*1.0 );
      // Create a new mutex every 1000 tasks. This is more relevant to the ThreadSchedulerMutexes; others ignore it.
      if (i % 1000 == 0)
        lastMutex = new Mutex();
      task->setMutex(lastMutex);
      p.schedule( task );
    }

    Timer overall;
    TS_ASSERT_THROWS_NOTHING( p.joinAll() );
    //std::cout << overall.elapsed() << " secs total." << std::endl;

    // Expected total
    size_t expected = (num * num + num) / 2;
    TS_ASSERT_EQUALS( mywaster.total, expected);
  }


  void test_StressTest_ThreadSchedulerFIFO()
  {
    do_StressTest_scheduler(new ThreadSchedulerFIFO());
  }

  void test_StressTest_ThreadSchedulerLIFO()
  {
    do_StressTest_scheduler(new ThreadSchedulerLIFO());
  }

  void test_StressTest_ThreadSchedulerLargestCost()
  {
    do_StressTest_scheduler(new ThreadSchedulerLargestCost());
  }

  void test_StressTest_ThreadSchedulerMutexes()
  {
    do_StressTest_scheduler(new ThreadSchedulerMutexes());
  }


  //--------------------------------------------------------------------
  /** Perform a stress test on the given scheduler.
   * This one creates tasks that create new tasks; e.g. 10 tasks each add
   * 10 tasks, and so on (up to a certain depth).
   * So it tests against possible segfaults of one task
   * accessing the queue while another thread is popping it.
   */
  void do_StressTest_TasksThatCreateTasks(ThreadScheduler * sched)
  {
    ThreadPool * p = new ThreadPool(sched, 0);
    // Create the first task, depth 0, that will recursively create 10000
    TaskThatAddsTasks * task = new TaskThatAddsTasks(sched, 0);
    p->schedule( task );

    //Reset the total
    TaskThatAddsTasks_counter = 0;
    TS_ASSERT_THROWS_NOTHING( p->joinAll() );

    // Expected total = the number of lowest level entries
    TS_ASSERT_EQUALS( TaskThatAddsTasks_counter, 10000);
    delete p;
  }

  void test_StressTest_TasksThatCreateTasks_ThreadSchedulerFIFO()
  {
    do_StressTest_TasksThatCreateTasks(new ThreadSchedulerFIFO());
  }

  void test_StressTest_TasksThatCreateTasks_ThreadSchedulerLIFO()
  {
    do_StressTest_TasksThatCreateTasks(new ThreadSchedulerLIFO());
  }

  void test_StressTest_TasksThatCreateTasks_ThreadSchedulerLargestCost()
  {
    do_StressTest_TasksThatCreateTasks(new ThreadSchedulerLargestCost());
  }

  void test_StressTest_TasksThatCreateTasks_ThreadSchedulerMutexes()
  {
    do_StressTest_TasksThatCreateTasks(new ThreadSchedulerMutexes());
  }

  //=======================================================================================
  /** Task that throws an exception */
  class TaskThatThrows : public Task
  {
    void run()
    {
      ThreadPoolTest_TaskThatThrows_counter++;
      throw Mantid::Kernel::Exception::NotImplementedError("Test exception from TaskThatThrows.");
    }
  };

  //--------------------------------------------------------------------
  void test_TaskThatThrows()
  {
    ThreadPool p(new ThreadSchedulerFIFO(),1); // one core
    ThreadPoolTest_TaskThatThrows_counter = 0;
    for (int i=0; i< 10; i++)
    {
      double cost = i;
      p.schedule( new TaskThatThrows());
    }
    // joinAll rethrows
    TS_ASSERT_THROWS( p.joinAll() , std::runtime_error);
    // And only one of the tasks actually ran (since we're on one core)
    TS_ASSERT_EQUALS(ThreadPoolTest_TaskThatThrows_counter, 1);
  }

};

#endif

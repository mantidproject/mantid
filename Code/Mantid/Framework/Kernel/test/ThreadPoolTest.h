#ifndef THREADPOOLTEST_H_
#define THREADPOOLTEST_H_

#include <cxxtest/TestSuite.h>

#include <MantidKernel/Timer.h>
#include <MantidKernel/FunctionTask.h>
#include "MantidKernel/MultiThreaded.h"
#include <MantidKernel/ThreadPool.h>
#include "MantidKernel/ThreadScheduler.h"

#include <boost/bind.hpp>
#include <iostream>
#include <iomanip>
#include <Poco/Mutex.h>

using namespace Mantid::Kernel;

//#include <boost/thread.hpp>


class TimeWaster
{
public:
static size_t waste_time(double seconds)
{
  // Waste time, but use up the CPU!
  std::size_t num = 0;
  Mantid::Kernel::Timer time;
  while (time.elapsed_no_reset() < seconds)
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
    size_t num = 100000;
    mywaster.total = 0;
    for (size_t i=0; i<=num; i++)
    {
      p.schedule( new FunctionTask( boost::bind(&TimeWaster::add_to_number, &mywaster, i), i*1.0 ) );
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


};

#endif

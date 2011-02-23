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

using namespace Mantid::Kernel;

//#include <boost/thread.hpp>



size_t waste_time(double seconds)
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
      waste_time(delay);
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




};

#endif

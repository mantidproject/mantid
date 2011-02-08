#ifndef THREADPOOLTEST_H_
#define THREADPOOLTEST_H_

#include <cxxtest/TestSuite.h>

#include <MantidKernel/Timer.h>
#include "MantidKernel/MultiThreaded.h"
#include <MantidKernel/ThreadPool.h>

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




class ThreadPoolTest : public CxxTest::TestSuite
{
public:
  void setUp()
  {
  }
  
  void tearDown()
  {
  }
  

  void test_Constructor()
  {
    Mantid::Kernel::Timer overall;
    for (int i=0; i<1000000; i++)
    {
      ThreadPool::Instance().test();
    }
    //std::cout << std::setw(15) << overall.elapsed() << " secs total" << std::endl;
  }


  /** Test that shows that OPENMP does not use a thread pool idea to optimally allocate threads */
  void xtestOpenMP()
  {
    Timer overall;
    int num = 16;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i<num; i++)
    {
      double delay = i;
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

};

#endif

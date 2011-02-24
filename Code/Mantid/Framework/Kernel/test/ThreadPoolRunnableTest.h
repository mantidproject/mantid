#ifndef MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_
#define MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidKernel/ThreadPoolRunnable.h>
#include <MantidKernel/ThreadScheduler.h>

using namespace Mantid::Kernel;

int ThreadPoolRunnableTest_value;

class ThreadPoolRunnableTest : public CxxTest::TestSuite
{
public:




  void test_constructor()
  {
    ThreadPoolRunnable * tpr;
    TS_ASSERT_THROWS( tpr = new ThreadPoolRunnable(0, NULL), std::invalid_argument);
  }

  //=======================================================================================
  class SimpleTask : public Task
  {
    void run()
    {
      ThreadPoolRunnableTest_value = 1234;
    }
  };

  void test_run()
  {
    ThreadPoolRunnable * tpr;
    ThreadScheduler * sc = new ThreadSchedulerFIFO();
    tpr = new ThreadPoolRunnable(0, sc);
    sc->push(new SimpleTask());
    TS_ASSERT_EQUALS( sc->size(), 1 );

    // Run it
    ThreadPoolRunnableTest_value = 0;
    tpr->run();

    // The task worked
    TS_ASSERT_EQUALS( ThreadPoolRunnableTest_value, 1234);
    // Nothing more in the queue.
    TS_ASSERT_EQUALS( sc->size(), 0 );
  }


  //=======================================================================================
  /** Class that throws an exception */
  class TaskThatThrows : public Task
  {
    void run()
    {
      ThreadPoolRunnableTest_value += 1;
      throw Mantid::Kernel::Exception::NotImplementedError("Test exception from TaskThatThrows.");
    }
  };

  void test_run_throws()
  {
    ThreadPoolRunnable * tpr;
    ThreadScheduler * sc = new ThreadSchedulerFIFO();
    tpr = new ThreadPoolRunnable(0, sc);

    // Put 10 tasks in
    for (size_t i=0; i<10; i++)
      sc->push(new TaskThatThrows());

    // The task throws but the runnable just aborts instead
    ThreadPoolRunnableTest_value = 0;
    TS_ASSERT_THROWS_NOTHING(tpr->run());

    // Nothing more in the queue.
    TS_ASSERT_EQUALS( sc->size(), 0 );
    // Yet only one task actually ran.
    TS_ASSERT_EQUALS( ThreadPoolRunnableTest_value, 1 );

    TS_ASSERT( sc->getAborted() );
    // Get the reason for aborting
    std::runtime_error e = sc->getAbortException();
    std::string what = e.what();
    TS_ASSERT_EQUALS(what, "Test exception from TaskThatThrows.");

  }




};


#endif /* MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_ */


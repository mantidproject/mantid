#ifndef MANTID_KERNEL_THREADSCHEDULERTEST_H_
#define MANTID_KERNEL_THREADSCHEDULERTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidKernel/ThreadScheduler.h>
#include <MantidKernel/Task.h>

using namespace Mantid::Kernel;

int ThreadSchedulerTest_numDestructed;

class ThreadSchedulerTest : public CxxTest::TestSuite
{
public:

  class TaskDoNothing : public Task
  {
  public:
    TaskDoNothing() : Task() {}
    TaskDoNothing(double cost) : Task() {m_cost = cost;}

    ~TaskDoNothing()
    {
      // To keep track of proper deleting of Task pointers
      ThreadSchedulerTest_numDestructed++;
    }

    void run() {}
  };

  void do_basic_test(ThreadScheduler * sc)
  {
    ThreadSchedulerTest_numDestructed = 0;
    TS_ASSERT( !sc->getAborted());
    TS_ASSERT_EQUALS( std::string(sc->getAbortException().what()), "");
    TS_ASSERT_EQUALS( sc->size(), 0 );

    sc->push( new TaskDoNothing() );
    TS_ASSERT_EQUALS( sc->size(), 1 );
    sc->push( new TaskDoNothing() );
    TS_ASSERT_EQUALS( sc->size(), 2 );

    // Clear empties the queue
    ThreadSchedulerTest_numDestructed = 0;
    sc->clear();
    TS_ASSERT_EQUALS( sc->size(), 0 );
    // And deletes the tasks properly
    TS_ASSERT_EQUALS(ThreadSchedulerTest_numDestructed, 2);

    delete sc;
  }


  void test_basic_ThreadSchedulerFIFO()
  {
    do_basic_test(new ThreadSchedulerFIFO());
  }

  void test_basic_ThreadSchedulerLIFO()
  {
    do_basic_test(new ThreadSchedulerLIFO());
  }

  void test_basic_ThreadSchedulerLargestCost()
  {
    do_basic_test(new ThreadSchedulerLargestCost());
  }


  //==================================================================================================

  void do_test(ThreadScheduler * sc, double * costs, size_t * poppedIndices)
  {
    ThreadSchedulerTest_numDestructed = 0;

    // Create and push them in order
    TaskDoNothing * tasks[4];
    for (size_t i=0; i<4; i++)
    {
      tasks[i] = new TaskDoNothing(costs[i]);
      sc->push(tasks[i]);
    }

    // Pop them, and check that we get them in the order we expected
    for (size_t i=0; i<4; i++)
    {
      TaskDoNothing * task = dynamic_cast<TaskDoNothing *>(sc->pop(0));
      size_t index=0;
      for (index=0; index<4; index++)
        if (task == tasks[index]) break;
      TS_ASSERT_EQUALS( index, poppedIndices[i]);
    }

    // Nothing is left
    TS_ASSERT_EQUALS(sc->size(), 0);

    // And ThreadScheduler does not delete popped tasks in this way
    TS_ASSERT_EQUALS(ThreadSchedulerTest_numDestructed, 0);
  }

  void test_ThreadSchedulerFIFO()
  {
    ThreadScheduler * sc = new ThreadSchedulerFIFO();
    double costs[4] = {0,1,2,3};
    size_t poppedIndices[4] = {0,1,2,3};
    do_test(sc, costs, poppedIndices);
  }

  void test_ThreadSchedulerLIFO()
  {
    ThreadScheduler * sc = new ThreadSchedulerLIFO();
    double costs[4] = {0,1,2,3};
    size_t poppedIndices[4] = {3,2,1,0};
    do_test(sc, costs, poppedIndices);
  }

  void test_ThreadSchedulerLargestCost()
  {
    ThreadScheduler * sc = new ThreadSchedulerLargestCost();
    double costs[4] = {1,5,2,-3};
    size_t poppedIndices[4] = {1,2,0,3};
    do_test(sc, costs, poppedIndices);
  }


};


#endif /* MANTID_KERNEL_THREADSCHEDULERTEST_H_ */


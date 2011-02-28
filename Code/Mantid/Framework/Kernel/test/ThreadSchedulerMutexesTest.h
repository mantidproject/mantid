#ifndef MANTID_KERNEL_THREADSCHEDULERMUTEXESTEST_H_
#define MANTID_KERNEL_THREADSCHEDULERMUTEXESTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidKernel/ThreadSchedulerMutexes.h>

using namespace Mantid::Kernel;

int ThreadSchedulerMutexesTest_timesDeleted;

class ThreadSchedulerMutexesTest : public CxxTest::TestSuite
{
public:

  /** A custom implementation of Task,
   * that sets its mutex */
  class TaskWithMutex : public Task
  {
  public:
    TaskWithMutex(Mutex * mutex, double cost)
    {
      m_mutex = mutex;
      m_cost = cost;
    }

    /// Count # of times destructed in the destructor
    ~TaskWithMutex()
    {ThreadSchedulerMutexesTest_timesDeleted++;}

    void run()
    {
      //TaskTestNamespace::my_check_value = 123;
    }
  };


  void test_push()
  {
    ThreadSchedulerMutexes sc;
    Mutex * mut1 = new Mutex();
    Mutex * mut2 = new Mutex();
    TaskWithMutex * task1 = new TaskWithMutex(mut1, 10.0);
    TaskWithMutex * task2 = new TaskWithMutex(mut2,  9.0);

    sc.push( task1 );
    TS_ASSERT_EQUALS( sc.size(), 1);
    sc.push( task2 );
    TS_ASSERT_EQUALS( sc.size(), 2);
  }

  void test_queue()
  {
    ThreadSchedulerMutexes sc;
    Mutex * mut1 = new Mutex();
    Mutex * mut2 = new Mutex();
    Mutex * mut3 = new Mutex();
    TaskWithMutex * task1 = new TaskWithMutex(mut1, 10.0);
    TaskWithMutex * task2 = new TaskWithMutex(mut1,  9.0);
    TaskWithMutex * task3 = new TaskWithMutex(mut1,  8.0);
    TaskWithMutex * task4 = new TaskWithMutex(mut2,  7.0);
    TaskWithMutex * task5 = new TaskWithMutex(mut2,  6.0);
    TaskWithMutex * task6 = new TaskWithMutex(mut3,  5.0);
    TaskWithMutex * task7 = new TaskWithMutex(NULL,  4.0);
    sc.push(task1);
    sc.push(task2);
    sc.push(task3);
    TS_ASSERT_EQUALS( sc.size(), 3);

    Task * task;
    // Run the first task. mut1 becomes busy
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task1 );
    TS_ASSERT_EQUALS( sc.size(), 2);

    // Add some tasks with mut2
    sc.push(task4);
    sc.push(task5);
    TS_ASSERT_EQUALS( sc.size(), 4);

    // Next one will be task4 since mut1 is locked. mut2 is busy now too.
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task4 );
    TS_ASSERT_EQUALS( sc.size(), 3);

    sc.push(task6);

    // Next one will be task6 since mut1 and mut2 are locked. mut3 is busy now too.
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task6 );
    TS_ASSERT_EQUALS( sc.size(), 3);

    // This task has NO mutex, so it comes next
    sc.push(task7);
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task7 );
    TS_ASSERT_EQUALS( sc.size(), 3);

    // Now we release task1, allowing task2 to come next
    sc.finished(task1,0);
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task2 );
    TS_ASSERT_EQUALS( sc.size(), 2);
    sc.finished(task2,0); // Have to complete task2 before task3 comes
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task3 );
    TS_ASSERT_EQUALS( sc.size(), 1);

    // mut2 is still locked, but since it's the last one, task5 is returned
    task = sc.pop(0);
    TS_ASSERT_EQUALS( task, task5 );
    TS_ASSERT_EQUALS( sc.size(), 0 );
    // (for this task, the thread pool would have to wait till the mutex is released)

  }

  void test_clear()
  {
    ThreadSchedulerMutexes sc;
    for (size_t i=0; i<10; i++)
    {
      TaskWithMutex * task = new TaskWithMutex(new Mutex(), 10.0);
      sc.push(task);
    }
    TS_ASSERT_EQUALS(sc.size(), 10);
    ThreadSchedulerMutexesTest_timesDeleted = 0;
    sc.clear();
    TS_ASSERT_EQUALS(sc.size(), 0);
    // Was the destructor called enough times?
    TS_ASSERT_EQUALS(ThreadSchedulerMutexesTest_timesDeleted, 10);
  }

  void test_performance_same_mutex()
  {
    ThreadSchedulerMutexes sc;
    Timer tim0;
    Mutex * mut1 = new Mutex();
    size_t num = 500;
    for (size_t i=0; i < num; i++)
    {
      sc.push(new TaskWithMutex(mut1, 10.0));
    }
    //std::cout << tim0.elapsed() << " secs to push." << std::endl;
    TS_ASSERT_EQUALS( sc.size(), num);

    Timer tim1;
    for (size_t i=0; i < num; i++)
    {
      Task * task;
      task = sc.pop(0);
    }
    //std::cout << tim1.elapsed() << " secs to pop." << std::endl;
    TS_ASSERT_EQUALS( sc.size(), 0);
  }

  void test_performance_lotsOfMutexes()
  {
    ThreadSchedulerMutexes sc;
    Timer tim0;
    size_t num = 500;
    for (size_t i=0; i < num; i++)
    {
      sc.push(new TaskWithMutex(new Mutex(), 10.0));
    }
    //std::cout << tim0.elapsed() << " secs to push." << std::endl;
    TS_ASSERT_EQUALS( sc.size(), num);

    Timer tim1;
    for (size_t i=0; i < num; i++)
    {
      Task * task;
      task = sc.pop(0);
    }
    //std::cout << tim1.elapsed() << " secs to pop." << std::endl;
    TS_ASSERT_EQUALS( sc.size(), 0);
  }


};


#endif /* MANTID_KERNEL_THREADSCHEDULERMUTEXESTEST_H_ */


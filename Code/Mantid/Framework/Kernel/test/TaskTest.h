#ifndef TASKTEST_H_
#define TASKTEST_H_

#include <cxxtest/TestSuite.h>

#include <MantidKernel/Timer.h>
#include "MantidKernel/Task.h"

#include <iostream>
#include <iomanip>

using namespace Mantid::Kernel;

namespace TaskTestNamespace
{
int my_check_value = 0;
}


/** A custom implementation of Task */
class MyTask : public Task
{
public:
  void run()
  {
    TaskTestNamespace::my_check_value = 123;
  }
};


class TaskTest : public CxxTest::TestSuite
{
public:
  void test_run()
  {
    MyTask t;
    TaskTestNamespace::my_check_value = 0;
    TS_ASSERT_DIFFERS( TaskTestNamespace::my_check_value, 123 );
    t.run();
    TS_ASSERT_EQUALS( TaskTestNamespace::my_check_value, 123 );
  }

  void test_mutex()
  {
    MyTask t;
    Mutex * mut = new Mutex();
    t.setMutex(mut);
    TS_ASSERT_EQUALS( mut, t.getMutex() );
  }



};

#endif

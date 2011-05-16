#ifndef FUNCTIONTASKTEST_H_
#define FUNCTIONTASKTEST_H_

#include <cxxtest/TestSuite.h>

#include <MantidKernel/Timer.h>
#include "MantidKernel/Task.h"
#include "MantidKernel/FunctionTask.h"

#include <iostream>
#include <iomanip>

using namespace Mantid::Kernel;

/** Functions for use by the tasks */
int my_check_value = 0;

void my_void_function()
{
  my_check_value = 12;
}

void my_int_function(int arg)
{
  my_check_value = arg;
}

double my_complicated_function(int arg1, double arg2)
{
  my_check_value = arg1 + static_cast<int>(arg2);
  return -1.0;
}


class FunctionTaskTest : public CxxTest::TestSuite
{
public:

  void test_NullFunction_throws()
  {
    FunctionTask mytask(NULL);
    TS_ASSERT_THROWS( mytask.run(), std::runtime_error );
  }

  void test_VoidFunction()
  {
    FunctionTask mytask(my_void_function);
    TS_ASSERT_EQUALS(my_check_value, 0);
    TS_ASSERT_THROWS_NOTHING( mytask.run() );
    TS_ASSERT_EQUALS(my_check_value, 12);
  }

  void test_Function_using_bind()
  {
    FunctionTask mytask( boost::bind<void, int>(my_int_function, 34)  );
    TS_ASSERT_DIFFERS(my_check_value, 34);
    TS_ASSERT_THROWS_NOTHING( mytask.run() );
    TS_ASSERT_EQUALS(my_check_value, 34);
  }

  void test_Function_using_bind_complicated()
  {
    FunctionTask mytask( boost::bind(my_complicated_function, 56, 12.0)  );
    TS_ASSERT_DIFFERS(my_check_value, 68);
    TS_ASSERT_THROWS_NOTHING( mytask.run() );
    TS_ASSERT_EQUALS(my_check_value, 68);
  }


};

#endif

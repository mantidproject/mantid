#ifndef MANTID_KERNEL_VECTORHELPERTEST_H_
#define MANTID_KERNEL_VECTORHELPERTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VectorHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::Kernel;

class VectorHelperTest : public CxxTest::TestSuite
{
public:

  // TODO: More tests of other methods

  void test_splitStringIntoVector()
  {
    std::vector<int> vec = VectorHelper::splitStringIntoVector<int>("1,2,-5,23");
    TS_ASSERT_EQUALS( vec.size(), 4);
    TS_ASSERT_EQUALS( vec[0], 1);
    TS_ASSERT_EQUALS( vec[1], 2);
    TS_ASSERT_EQUALS( vec[2], -5);
    TS_ASSERT_EQUALS( vec[3], 23);
  }

  void test_splitStringIntoVector_empty()
  {
    std::vector<int> vec = VectorHelper::splitStringIntoVector<int>("");
    TS_ASSERT_EQUALS( vec.size(), 0);
    vec = VectorHelper::splitStringIntoVector<int>(",   ,  ,");
    TS_ASSERT_EQUALS( vec.size(), 0);
  }

  void test_splitStringIntoVector_double()
  {
    std::vector<double> vec = VectorHelper::splitStringIntoVector<double>("1.234, 2.456");
    TS_ASSERT_EQUALS( vec.size(), 2);
    TS_ASSERT_DELTA( vec[0], 1.234, 1e-5);
    TS_ASSERT_DELTA( vec[1], 2.456, 1e-5);
  }

  void test_splitStringIntoVector_string()
  {
    std::vector<std::string> vec = VectorHelper::splitStringIntoVector<std::string>("Hey, Jude");
    TS_ASSERT_EQUALS( vec.size(), 2);
    TS_ASSERT_EQUALS( vec[0], "Hey");
    TS_ASSERT_EQUALS( vec[1], "Jude");
  }

  void test_splitStringIntoVector_badNumber_gives0()
  {
    std::vector<int> vec;
    vec = VectorHelper::splitStringIntoVector<int>("2, monkey, potato, 134");
    TS_ASSERT_EQUALS( vec.size(), 4);
    TS_ASSERT_EQUALS( vec[0], 2);
    TS_ASSERT_EQUALS( vec[1], 0);
    TS_ASSERT_EQUALS( vec[2], 0);
    TS_ASSERT_EQUALS( vec[3], 134);
  }


};


#endif /* MANTID_KERNEL_VECTORHELPERTEST_H_ */


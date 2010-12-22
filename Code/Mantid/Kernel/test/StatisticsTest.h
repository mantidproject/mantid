#ifndef STATISTICSTEST_H_
#define STATISTICSTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <string>
#include "MantidKernel/Statistics.h"

using namespace Mantid::Kernel;
using std::string;
using std::vector;

class StatisticsTest : public CxxTest::TestSuite
{
public:
  void testDoubleOdd()
  {
    vector<double> data;
    data.push_back(17.2);
    data.push_back(18.1);
    data.push_back(16.5);
    data.push_back(18.3);
    data.push_back(12.6);

    Statistics stats = getStatistics(data);

    TS_ASSERT_EQUALS(stats.mean, 16.54);
    TS_ASSERT_DELTA(stats.standard_deviation, 2.0732, 0.0001);
    TS_ASSERT_EQUALS(stats.minimum, 12.6);
    TS_ASSERT_EQUALS(stats.maximum, 18.3);
    TS_ASSERT_EQUALS(stats.median, 17.2);
  }

  void testDoubleSingle()
  {
    vector<double> data;
    data.push_back(42.);

    Statistics stats = getStatistics(data);

    TS_ASSERT_EQUALS(stats.mean, 42.);
    TS_ASSERT_EQUALS(stats.standard_deviation, 0.);
    TS_ASSERT_EQUALS(stats.minimum, 42.);
    TS_ASSERT_EQUALS(stats.maximum, 42.);
    TS_ASSERT_EQUALS(stats.median, 42.);
  }

  void testInt32Even()
  {
    vector<int32_t> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(4);
    data.push_back(5);
    data.push_back(6);

    Statistics stats = getStatistics(data);

    TS_ASSERT_EQUALS(stats.mean, 3.5);
    TS_ASSERT_DELTA(stats.standard_deviation, 1.7078, 0.0001);
    TS_ASSERT_EQUALS(stats.minimum, 1.);
    TS_ASSERT_EQUALS(stats.maximum, 6.);
    TS_ASSERT_EQUALS(stats.median, 3.5);
  }

  bool my_isnan(const double number)
  {
    return number != number;
  }

  void testString()
  {
    vector<string> data;
    data.push_back("hi there");

    Statistics stats = getStatistics(data);

    TS_ASSERT(my_isnan(stats.mean));
    TS_ASSERT(my_isnan(stats.standard_deviation));
    TS_ASSERT(my_isnan(stats.minimum));
    TS_ASSERT(my_isnan(stats.maximum));
    TS_ASSERT(my_isnan(stats.median));
  }
};

#endif // STATISTICSTEST_H_

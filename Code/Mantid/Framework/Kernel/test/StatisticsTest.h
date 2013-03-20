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
  void testZscores()
  {
    vector<double> data;
    data.push_back(12);
    data.push_back(13);
    data.push_back(9);
    data.push_back(18);
    data.push_back(7);
    data.push_back(9);
    data.push_back(14);
    data.push_back(16);
    data.push_back(10);
    data.push_back(12);
    data.push_back(7);
    data.push_back(13);
    data.push_back(14);
    data.push_back(19);
    data.push_back(10);
    data.push_back(16);
    data.push_back(12);
    data.push_back(16);
    data.push_back(19);
    data.push_back(11);

    std::vector<double> Zscore = getZscore(data);
    TS_ASSERT_DELTA(Zscore[4], 1.6397, 0.0001);
    TS_ASSERT_DELTA(Zscore[6], 0.3223, 0.0001);
    std::vector<double> ZModscore = getModifiedZscore(data);
    TS_ASSERT_DELTA(ZModscore[4], 1.2365, 0.0001);
    TS_ASSERT_DELTA(ZModscore[6], 0.3372, 0.0001);
    
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

  /** Test function to calculate Rwp
    */
  void testRwp()
  {
    vector<double> obsY(4);
    vector<double> calY(4);
    vector<double> obsE(4);

    obsY[0] = 1.0;
    calY[0] = 1.1;
    obsE[0] = 1.0;

    obsY[1] = 2.0;
    calY[1] = 2.1;
    obsE[1] = 1.2;

    obsY[2] = 3.0;
    calY[2] = 3.5;
    obsE[2] = 1.4;

    obsY[3] = 1.0;
    calY[3] = 1.3;
    obsE[3] = 1.0;

    double rwp = getRFactor(obsY, calY, obsE);

    TS_ASSERT_DELTA(rwp, 0.1582, 0.0001);
  }

  /** Test throw exception
    */
  void testRwpException1()
  {
    vector<double> obsY(4);
    vector<double> calY(4);
    vector<double> obsE(3);

    obsY[0] = 1.0;
    calY[0] = 1.1;
    obsE[0] = 1.0;

    obsY[1] = 2.0;
    calY[1] = 2.1;
    obsE[1] = 1.2;

    obsY[2] = 3.0;
    calY[2] = 3.5;
    obsE[2] = 1.4;

    obsY[3] = 1.0;
    calY[3] = 1.3;

    TS_ASSERT_THROWS_ANYTHING(getRFactor(obsY, calY, obsE));
  }

  /** Test throw exception on empty array
    */
  void testRwpException2()
  {
    vector<double> obsY;
    vector<double> calY;
    vector<double> obsE;

    TS_ASSERT_THROWS_ANYTHING(getRFactor(obsY, calY, obsE));
  }

};

#endif // STATISTICSTEST_H_

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef STATISTICSTEST_H_
#define STATISTICSTEST_H_

#include "MantidKernel/Statistics.h"
#include <algorithm>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <string>
#include <vector>

using namespace Mantid::Kernel;
using std::string;
using std::vector;

class StatisticsTest : public CxxTest::TestSuite {
public:
  void test_Doubles_And_Default_Flags_Calculates_All_Stats() {
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

  void test_Doubles_With_Sorted_Data() {
    vector<double> data;
    data.push_back(17.2);
    data.push_back(18.1);
    data.push_back(16.5);
    data.push_back(18.3);
    data.push_back(12.6);
    sort(data.begin(), data.end());

    Statistics stats =
        getStatistics(data, (StatOptions::Median | StatOptions::SortedData));

    TS_ASSERT(std::isnan(stats.mean));
    TS_ASSERT(std::isnan(stats.standard_deviation));
    TS_ASSERT(std::isnan(stats.minimum));
    TS_ASSERT(std::isnan(stats.maximum));
    TS_ASSERT_EQUALS(stats.median, 17.2);
  }

  void
  test_Unsorted_Data_With_Sorted_Flag_Gives_Expected_Incorrect_Result_For_Median() {
    vector<double> data;
    data.push_back(17.2);
    data.push_back(18.1);
    data.push_back(16.5);
    data.push_back(18.3);
    data.push_back(12.6);

    Statistics stats =
        getStatistics(data, (StatOptions::Median | StatOptions::SortedData));

    TS_ASSERT(std::isnan(stats.mean));
    TS_ASSERT(std::isnan(stats.standard_deviation));
    TS_ASSERT(std::isnan(stats.minimum));
    TS_ASSERT(std::isnan(stats.maximum));
    TS_ASSERT_EQUALS(stats.median, 16.5);
  }

  void test_Doubles_With_Corrected_StdDev_Calculates_Mean() {
    vector<double> data;
    data.push_back(17.2);
    data.push_back(18.1);
    data.push_back(16.5);
    data.push_back(18.3);
    data.push_back(12.6);
    sort(data.begin(), data.end());

    Statistics stats = getStatistics(data, StatOptions::CorrectedStdDev);

    TS_ASSERT_EQUALS(stats.mean, 16.54);
    TS_ASSERT_DELTA(stats.standard_deviation, 2.3179, 0.0001);
    TS_ASSERT_EQUALS(stats.minimum, 12.6);
    TS_ASSERT_EQUALS(stats.maximum, 18.3);
    TS_ASSERT(std::isnan(stats.median));
  }

  void test_Types_Can_Be_Disabled_With_Flags() {
    vector<double> data;
    data.push_back(17.2);
    data.push_back(18.1);
    data.push_back(16.5);
    data.push_back(18.3);
    data.push_back(12.6);

    Statistics justMean = getStatistics(data, StatOptions::Mean);
    TS_ASSERT_EQUALS(justMean.mean, 16.54);
    TS_ASSERT(std::isnan(justMean.standard_deviation));
    TS_ASSERT(std::isnan(justMean.minimum));
    TS_ASSERT(std::isnan(justMean.maximum));
    TS_ASSERT(std::isnan(justMean.median));
  }

  void testZscores() {
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

  void testDoubleSingle() {
    vector<double> data;
    data.push_back(42.);

    Statistics stats = getStatistics(data);

    TS_ASSERT_EQUALS(stats.mean, 42.);
    TS_ASSERT_EQUALS(stats.standard_deviation, 0.);
    TS_ASSERT_EQUALS(stats.minimum, 42.);
    TS_ASSERT_EQUALS(stats.maximum, 42.);
    TS_ASSERT_EQUALS(stats.median, 42.);
  }

  void testInt32Even() {
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

  bool my_isnan(const double number) { return number != number; }

  void testString() {
    vector<string> data{"hi there"};

    Statistics stats = getStatistics(data);

    TS_ASSERT(my_isnan(stats.mean));
    TS_ASSERT(my_isnan(stats.standard_deviation));
    TS_ASSERT(my_isnan(stats.minimum));
    TS_ASSERT(my_isnan(stats.maximum));
    TS_ASSERT(my_isnan(stats.median));
  }

  /** Test function to calculate Rwp
   */
  void testRwp() {
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

    Rfactor rfactor = getRFactor(obsY, calY, obsE);

    TS_ASSERT_DELTA(rfactor.Rwp, 0.1582, 0.0001);
  }

  /** Test throw exception
   */
  void testRwpException1() {
    vector<double> obsY{1.0, 2.0, 3.0, 1.0};
    vector<double> calY{1.1, 2.1, 3.5, 1.3};
    vector<double> obsE{1.0, 1.2, 1.4};

    TS_ASSERT_THROWS_ANYTHING(getRFactor(obsY, calY, obsE));
  }

  /** Test throw exception on empty array
   */
  void testRwpException2() {
    vector<double> obsY;
    vector<double> calY;
    vector<double> obsE;

    TS_ASSERT_THROWS_ANYTHING(getRFactor(obsY, calY, obsE));
  }

  /// Test moment calculations about origin and mean
  void test_getMoments() {
    const double mean = 5.;
    const double sigma = 4.;
    const double deltaX = .2;
    const size_t numX = 200;
    // calculate to have same number of points left and right of function
    const double offsetX = mean - (.5 * deltaX * static_cast<double>(numX));
    // variance about origin
    double expVar = mean * mean + sigma * sigma;
    // skew about origin
    double expSkew = mean * mean * mean + 3. * mean * sigma * sigma;

    // x-values to try out
    vector<double> x;
    for (size_t i = 0; i < numX; ++i)
      x.push_back(static_cast<double>(i) * deltaX + offsetX);

    // just declare so we can have test of exception handling
    vector<double> y;

    TS_ASSERT_THROWS(getMomentsAboutOrigin(x, y), const std::out_of_range &);

    // now calculate the y-values
    for (size_t i = 0; i < numX; ++i) {
      double temp = (x[i] - mean) / sigma;
      y.push_back(exp(-.5 * temp * temp) / (sigma * sqrt(2. * M_PI)));
    }

    // Normal distribution values are taken from the wikipedia page
    {
      std::cout << "Normal distribution about origin\n";
      vector<double> aboutOrigin = getMomentsAboutOrigin(x, y);
      TS_ASSERT_EQUALS(aboutOrigin.size(), 4);
      TS_ASSERT_DELTA(aboutOrigin[0], 1., .0001);
      TS_ASSERT_DELTA(aboutOrigin[1], mean, .0001);
      TS_ASSERT_DELTA(aboutOrigin[2], expVar, .001 * expVar);
      TS_ASSERT_DELTA(aboutOrigin[3], expSkew, .001 * expSkew);

      std::cout << "Normal distribution about mean\n";
      vector<double> aboutMean = getMomentsAboutMean(x, y);
      TS_ASSERT_EQUALS(aboutMean.size(), 4);
      TS_ASSERT_DELTA(aboutMean[0], 1., .0001);
      TS_ASSERT_DELTA(aboutMean[1], 0., .0001);
      TS_ASSERT_DELTA(aboutMean[2], sigma * sigma, .001 * expVar);
      TS_ASSERT_DELTA(aboutMean[3], 0., .0001 * expSkew);
    }

    // Now a gaussian function as a histogram
    y.clear();
    for (size_t i = 0; i < numX - 1;
         ++i) // one less y than x makes it a histogram
    {
      double templeft = (x[i] - mean) / sigma;
      templeft = exp(-.5 * templeft * templeft) / (sigma * sqrt(2. * M_PI));
      double tempright = (x[i + 1] - mean) / sigma;
      tempright = exp(-.5 * tempright * tempright) / (sigma * sqrt(2. * M_PI));
      y.push_back(.5 * deltaX * (templeft + tempright));
      //      std::cout << i << ":\t" << x[i] << "\t" << y[i] << '\n';
    }

    // Normal distribution values are taken from the wikipedia page
    {
      std::cout << "Normal distribution about origin\n";
      vector<double> aboutOrigin = getMomentsAboutOrigin(x, y);
      TS_ASSERT_EQUALS(aboutOrigin.size(), 4);
      TS_ASSERT_DELTA(aboutOrigin[0], 1., .0001);
      TS_ASSERT_DELTA(aboutOrigin[1], mean, .0001);
      TS_ASSERT_DELTA(aboutOrigin[2], expVar, .001 * expVar);
      TS_ASSERT_DELTA(aboutOrigin[3], expSkew, .001 * expSkew);

      std::cout << "Normal distribution about mean\n";
      vector<double> aboutMean = getMomentsAboutMean(x, y);
      TS_ASSERT_EQUALS(aboutMean.size(), 4);
      TS_ASSERT_DELTA(aboutMean[0], 1., .0001);
      TS_ASSERT_DELTA(aboutMean[1], 0., .0001);
      TS_ASSERT_DELTA(aboutMean[2], sigma * sigma, .001 * expVar);
      TS_ASSERT_DELTA(aboutMean[3], 0., .0001 * expSkew);
    }
  }
};

#endif // STATISTICSTEST_H_

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
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VectorHelperTest *createSuite() { return new VectorHelperTest(); }
  static void destroySuite( VectorHelperTest *suite ) { delete suite; }

  VectorHelperTest() : m_test_bins(5,0.0)
  {
    m_test_bins[0] = -1.1;
    m_test_bins[1] = -0.2;
    m_test_bins[2] = 0.7;
    m_test_bins[3] = 1.6;
    m_test_bins[4] = 3.2;
  }

  void test_CreateAxisFromRebinParams_Gives_Expected_Number_Bins()
  {
    std::vector<double> rbParams(3);
    rbParams[0] = 1;
    rbParams[1] = 1;
    rbParams[2] = 10;

    std::vector<double> axis;
    const int numBoundaries = VectorHelper::createAxisFromRebinParams(rbParams,axis);

    TS_ASSERT_EQUALS(numBoundaries, 10);
    TS_ASSERT_EQUALS(axis.size(), 10);
  }

  void test_CreateAxisFromRebinParams_Gives_Expected_Number_Bins_But_Not_Resized_Axis_When_Requested()
  {
    std::vector<double> rbParams(3);
    rbParams[0] = 1;
    rbParams[1] = 1;
    rbParams[2] = 10;

    std::vector<double> axis;
    const int numBoundaries = VectorHelper::createAxisFromRebinParams(rbParams,axis, false);

    TS_ASSERT_EQUALS(numBoundaries, 10);
    TS_ASSERT_EQUALS(axis.size(), 0);
  }

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

  void test_normalizeVector_and_length()
  {
    std::vector<double> x;
    std::vector<double> y;
    TS_ASSERT_DELTA( VectorHelper::lengthVector(x), 0.0, 1e-5);

    y = VectorHelper::normalizeVector(x);
    TSM_ASSERT_EQUALS( "Pass-through empty vectors", y.size(), 0);
    x.push_back(3.0);
    x.push_back(4.0);
    TS_ASSERT_DELTA( VectorHelper::lengthVector(x), 5.0, 1e-5);
    y = VectorHelper::normalizeVector(x);
    TS_ASSERT_EQUALS( y.size(), 2);
    TS_ASSERT_DELTA( y[0], 0.6, 1e-5);
    TS_ASSERT_DELTA( y[1], 0.8, 1e-5);

    // Handle 0-length
    x[0] = 0.0;
    x[1] = 0.0;
    TS_ASSERT_DELTA( VectorHelper::lengthVector(x), 0.0, 1e-5);
    y = VectorHelper::normalizeVector(x);
    TS_ASSERT_EQUALS( y.size(), 2);
  }

  // TODO: Figure out proper behavior if given stupidity as inputs
//  void test_splitStringIntoVector_badNumber_gives0()
//  {
//    std::vector<int> vec;
//    vec = VectorHelper::splitStringIntoVector<int>("2, monkey, potato, 134");
//    TS_ASSERT_EQUALS( vec.size(), 4);
//    TS_ASSERT_EQUALS( vec[0], 2);
//    TS_ASSERT_EQUALS( vec[1], 0);
//    TS_ASSERT_EQUALS( vec[2], 0);
//    TS_ASSERT_EQUALS( vec[3], 134);
//  }

  void test_getBinIndex_Returns_Zero_For_Value_Lower_Than_Input_Range()
  {
    const double testValue = m_test_bins.front() - 1.1;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 0);
  }

  void test_getBinIndex_Returns_Zero_For_Value_Equal_To_Lowest_In_Input_Range()
  {
    const double testValue = m_test_bins.front();
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 0);
  }

  void test_getBinIndex_Returns_Last_Bin_For_Value_Equal_To_Highest_In_Input_Range()
  {
    const double testValue = m_test_bins.back();
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 3);
  }

  void test_getBinIndex_Returns_Index_Of_Last_Bin_For_Value_Greater_Than_Input_Range()
  {
    const double testValue = m_test_bins.back() + 10.1;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 3);
  }

  void test_getBinIndex_Returns_Correct_Bins_Index_For_Value_Not_On_Edge()
  {
    const double testValue = m_test_bins[1] + 0.3;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 1);
  }

  void test_getBinIndex_Returns_Index_For_Bin_On_RHS_Of_Boundary_When_Given_Value_Is_Equal_To_A_Boundary()
  {
    const double testValue = m_test_bins[2];
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 2);
  }

private:
  /// Testing bins
  std::vector<double> m_test_bins;

};


#endif /* MANTID_KERNEL_VECTORHELPERTEST_H_ */


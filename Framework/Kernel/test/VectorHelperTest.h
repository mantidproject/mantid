// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <cstdlib>
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Kernel;

class VectorHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VectorHelperTest *createSuite() { return new VectorHelperTest(); }
  static void destroySuite(VectorHelperTest *suite) { delete suite; }

  VectorHelperTest() : m_test_bins(5, 0.0) {
    m_test_bins[0] = -1.1;
    m_test_bins[1] = -0.2;
    m_test_bins[2] = 0.7;
    m_test_bins[3] = 1.6;
    m_test_bins[4] = 3.2;
  }

  void test_indexOfFromEdges() {
    std::vector<double> single;
    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromEdges(single, 7.1), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - vector is empty");
    single.emplace_back(1.7);
    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromEdges(single, 4.8), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - requires at least two bin edges");

    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromEdges(m_test_bins, -1.2), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");

    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromEdges(m_test_bins, 3.3), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");

    TS_ASSERT_EQUALS(VectorHelper::indexOfValueFromEdges(m_test_bins, 0.55), 1);
  }

  void test_indexOfFromCenters() {
    std::vector<double> single;
    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromCenters(single, 5.9), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - vector is empty");
    single.emplace_back(2.5);
    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromCenters(single, 6.1), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");
    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromCenters(single, 1.9), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");
    TS_ASSERT_EQUALS(VectorHelper::indexOfValueFromCenters(single, 2.25), 0);

    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromCenters(m_test_bins, -1.56), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");

    TS_ASSERT_THROWS_EQUALS(VectorHelper::indexOfValueFromCenters(m_test_bins, 4.1), const std::out_of_range &e,
                            std::string(e.what()), "indexOfValue - value out of range");

    TS_ASSERT_EQUALS(VectorHelper::indexOfValueFromCenters(m_test_bins, -1.23), 0);
    TS_ASSERT_EQUALS(VectorHelper::indexOfValueFromCenters(m_test_bins, 3.98), 4);
    TS_ASSERT_EQUALS(VectorHelper::indexOfValueFromCenters(m_test_bins, 0.8), 2);
  }

  void test_CreateAxisFromRebinParams_Gives_Expected_Number_Bins() {
    std::vector<double> rbParams(3);
    rbParams[0] = 1;
    rbParams[1] = 1;
    rbParams[2] = 10;

    std::vector<double> axis;
    const int numBoundaries = VectorHelper::createAxisFromRebinParams(rbParams, axis);

    TS_ASSERT_EQUALS(numBoundaries, 10);
    TS_ASSERT_EQUALS(axis.size(), 10);
  }

  void test_CreateAxisFromRebinParams_Gives_Expected_Number_Bins_But_Not_Resized_Axis_When_Requested() {
    std::vector<double> rbParams(3);
    rbParams[0] = 1;
    rbParams[1] = 1;
    rbParams[2] = 10;

    std::vector<double> axis;
    const int numBoundaries = VectorHelper::createAxisFromRebinParams(rbParams, axis, false);

    TS_ASSERT_EQUALS(numBoundaries, 10);
    TS_ASSERT_EQUALS(axis.size(), 0);
  }

  void test_CreateAxisFromRebinParams_SingleStep() {
    std::vector<double> rbParams = {0, 2, 5};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true);

    std::vector<double> expectedAxis = {0, 2, 4, 5};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_CreateAxisFromRebinParams_SingleStep_LastBinTooSmall() {
    std::vector<double> rbParams = {0.0, 2.0, 4.1};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true);

    std::vector<double> expectedAxis = {0.0, 2.0, 4.1};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_createAxisFromRebinParams_reverseLog() {
    std::vector<double> rbParams = {1, -1, 37};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, false, 1, 37, true);

    std::vector<double> expectedAxis = {1, 22, 30, 34, 36, 37};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_createAxisFromRebinParams_reverseLogFullBins() {
    std::vector<double> rbParams = {1, -1, 37};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, false, 1, 37, true);

    std::vector<double> expectedAxis = {1, 22, 30, 34, 36, 37};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_createAxisFromRebinParams_reverseLogWithDiffStep() {
    std::vector<double> rbParams = {1, -2, 42};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, false, 1, 42, true);

    std::vector<double> expectedAxis = {1, 34, 40, 42};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_createAxisFromRebinParams_inverseSquareRoot() {
    std::vector<double> rbParams = {1, 1, 3.5};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, false, 1, 3.5, true, 0.5);

    std::vector<double> expectedAxis = {1, 2, 2.707106781, 3.28445705, 3.5};
    TS_ASSERT_DELTA(axis, expectedAxis, 1e-5);
  }

  void test_createAxisFromRebinParams_harmonicSeries() {
    std::vector<double> rbParams = {1, 1, 3};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, false, 1, 3, true, 1);

    std::vector<double> expectedAxis = {1, 2, 2.5, 2.833333, 3};
    TS_ASSERT_DELTA(axis, expectedAxis, 1e-5);
  }

  void test_CreateAxisFromRebinParams_MultipleSteps() {
    std::vector<double> rbParams = {0, 2, 5, 3, 10, 1, 12};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true);

    std::vector<double> expectedAxis = {0, 2, 4, 5, 8, 10, 11, 12};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_CreateAxisFromRebinParams_FullBinsOnly_SingleStep() {
    std::vector<double> rbParams = {0, 2, 5};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, true);

    std::vector<double> expectedAxis = {0, 2, 4};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_CreateAxisFromRebinParams_FullBinsOnly_MultipleSteps() {
    std::vector<double> rbParams = {0, 2, 5, 3, 10, 1, 12};

    std::vector<double> axis;
    VectorHelper::createAxisFromRebinParams(rbParams, axis, true, true);

    std::vector<double> expectedAxis = {0, 2, 4, 7, 10, 11, 12};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_CreateAxisFromRebinParams_ThrowsIfSingleParamNoHintsProvided() {
    const std::vector<double> rbParams = {1.0};
    std::vector<double> axis;
    TS_ASSERT_THROWS(VectorHelper::createAxisFromRebinParams(rbParams, axis), const std::runtime_error &)
  }

  void test_createAxisFromRebinParams_throwsOnInfiniteVal() {
    const std::vector<double> params = {1.0, INFINITY};
    std::vector<double> axis;
    TS_ASSERT_THROWS(VectorHelper::createAxisFromRebinParams(params, axis), const std::runtime_error &);
  }

  void test_createAxisFromRebinParams_throwsOnNaNVal() {
    const std::vector<double> params = {1.0, NAN};
    std::vector<double> axis;
    TS_ASSERT_THROWS(VectorHelper::createAxisFromRebinParams(params, axis), const std::runtime_error &);
  }

  void test_CreateAxisFromRebinParams_xMinXMaxHints() {
    const std::vector<double> rbParams = {1.0};
    std::vector<double> axis;
    TS_ASSERT_THROWS_NOTHING(VectorHelper::createAxisFromRebinParams(rbParams, axis, true, true, -5., 3.))
    const std::vector<double> expectedAxis = {-5, -4, -3, -2, -1, 0, 1, 2, 3};
    TS_ASSERT_EQUALS(axis, expectedAxis);
  }

  void test_ConvertToBinBoundary_EmptyInputVector() {
    std::vector<double> bin_centers;
    std::vector<double> bin_edges;
    VectorHelper::convertToBinBoundary(bin_centers, bin_edges);

    TS_ASSERT_EQUALS(bin_edges.size(), 0);
  }

  void test_ConvertToBinBoundary_Size1InputVector() {
    std::vector<double> bin_centers = {0.4};
    std::vector<double> bin_edges;
    VectorHelper::convertToBinBoundary(bin_centers, bin_edges);

    TS_ASSERT_EQUALS(bin_edges.size(), 2);
    // In lack of a better guess for the bin width it is set to 1.0.
    TS_ASSERT_DELTA(bin_edges[0], -0.1, 1e-12);
    TS_ASSERT_DELTA(bin_edges[1], 0.9, 1e-12);
  }

  void test_ConvertToBinBoundary_Size2InputVector() {
    std::vector<double> bin_centers = {0.5, 1.5};
    std::vector<double> bin_edges;

    VectorHelper::convertToBinBoundary(bin_centers, bin_edges);

    TS_ASSERT_EQUALS(bin_edges.size(), 3);
    TS_ASSERT_DELTA(bin_edges[0], 0.0, 1e-12);
    TS_ASSERT_DELTA(bin_edges[1], 1.0, 1e-12);
    TS_ASSERT_DELTA(bin_edges[2], 2.0, 1e-12);
  }

  void test_flattenContainer_EmptyInputVector() {
    const std::vector<std::vector<int>> emptyInput;
    const auto result = VectorHelper::flattenVector<int>(emptyInput);
    TS_ASSERT(result.empty());
  }

  void test_flattenContainer_SingleSubvectorWithMultipleValues() {
    std::vector<std::vector<int>> input;
    input.emplace_back(std::vector<int>{3, 1, -1, -3, -5});
    const std::vector<int> expected{3, 1, -1, -3, -5};
    const auto result = VectorHelper::flattenVector(input);
    TS_ASSERT_EQUALS(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], expected[i]);
    }
  }

  void test_flattenContainer_MultipleSubvectorsWithSingleValues() {
    std::vector<std::vector<int>> input;
    input.emplace_back(std::vector<int>{3});
    input.emplace_back(std::vector<int>{1});
    input.emplace_back(std::vector<int>{-1});
    input.emplace_back(std::vector<int>{-3});
    input.emplace_back(std::vector<int>{-5});
    const std::vector<int> expected{3, 1, -1, -3, -5};
    const auto result = VectorHelper::flattenVector(input);
    TS_ASSERT_EQUALS(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], expected[i]);
    }
  }

  void test_flattenContainer_VariableSizedSubvectors() {
    std::vector<std::vector<int>> input;
    input.emplace_back(std::vector<int>{3, 1});
    input.emplace_back(std::vector<int>{});
    input.emplace_back(std::vector<int>{-1});
    input.emplace_back(std::vector<int>{-3, -5});
    const std::vector<int> expected{3, 1, -1, -3, -5};
    const auto result = VectorHelper::flattenVector(input);
    TS_ASSERT_EQUALS(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], expected[i]);
    }
  }

  // TODO: More tests of other methods

  void test_splitStringIntoVector() {
    std::vector<int> vec = VectorHelper::splitStringIntoVector<int>("1,2,-5,23");
    TS_ASSERT_EQUALS(vec.size(), 4);
    TS_ASSERT_EQUALS(vec[0], 1);
    TS_ASSERT_EQUALS(vec[1], 2);
    TS_ASSERT_EQUALS(vec[2], -5);
    TS_ASSERT_EQUALS(vec[3], 23);
  }

  void test_splitStringIntoVector_empty() {
    std::vector<int> vec = VectorHelper::splitStringIntoVector<int>("");
    TS_ASSERT_EQUALS(vec.size(), 0);
    vec = VectorHelper::splitStringIntoVector<int>(",   ,  ,");
    TS_ASSERT_EQUALS(vec.size(), 0);
  }

  void test_splitStringIntoVector_double() {
    std::vector<double> vec = VectorHelper::splitStringIntoVector<double>("1.234, 2.456");
    TS_ASSERT_EQUALS(vec.size(), 2);
    TS_ASSERT_DELTA(vec[0], 1.234, 1e-5);
    TS_ASSERT_DELTA(vec[1], 2.456, 1e-5);
  }

  void test_splitStringIntoVector_string() {
    std::vector<std::string> vec = VectorHelper::splitStringIntoVector<std::string>("Hey, Jude");
    TS_ASSERT_EQUALS(vec.size(), 2);
    TS_ASSERT_EQUALS(vec[0], "Hey");
    TS_ASSERT_EQUALS(vec[1], "Jude");
  }

  void test_normalizeVector_and_length() {
    std::vector<double> x;
    std::vector<double> y;
    TS_ASSERT_DELTA(VectorHelper::lengthVector(x), 0.0, 1e-5);

    y = VectorHelper::normalizeVector(x);
    TSM_ASSERT_EQUALS("Pass-through empty vectors", y.size(), 0);
    x.emplace_back(3.0);
    x.emplace_back(4.0);
    TS_ASSERT_DELTA(VectorHelper::lengthVector(x), 5.0, 1e-5);
    y = VectorHelper::normalizeVector(x);
    TS_ASSERT_EQUALS(y.size(), 2);
    TS_ASSERT_DELTA(y[0], 0.6, 1e-5);
    TS_ASSERT_DELTA(y[1], 0.8, 1e-5);

    // Handle 0-length
    x[0] = 0.0;
    x[1] = 0.0;
    TS_ASSERT_DELTA(VectorHelper::lengthVector(x), 0.0, 1e-5);
    y = VectorHelper::normalizeVector(x);
    TS_ASSERT_EQUALS(y.size(), 2);
  }

  void test_getBinIndex_Returns_Zero_For_Value_Lower_Than_Input_Range() {
    const double testValue = m_test_bins.front() - 1.1;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 0);
  }

  void test_getBinIndex_Returns_Zero_For_Value_Equal_To_Lowest_In_Input_Range() {
    const double testValue = m_test_bins.front();
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 0);
  }

  void test_getBinIndex_Returns_Last_Bin_For_Value_Equal_To_Highest_In_Input_Range() {
    const double testValue = m_test_bins.back();
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 3);
  }

  void test_getBinIndex_Returns_Index_Of_Last_Bin_For_Value_Greater_Than_Input_Range() {
    const double testValue = m_test_bins.back() + 10.1;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 3);
  }

  void test_getBinIndex_Returns_Correct_Bins_Index_For_Value_Not_On_Edge() {
    const double testValue = m_test_bins[1] + 0.3;
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 1);
  }

  void test_getBinIndex_Returns_Index_For_Bin_On_RHS_Of_Boundary_When_Given_Value_Is_Equal_To_A_Boundary() {
    const double testValue = m_test_bins[2];
    int index(-1);

    TS_ASSERT_THROWS_NOTHING(index = VectorHelper::getBinIndex(m_test_bins, testValue));
    TS_ASSERT_EQUALS(index, 2);
  }
  void test_RunningAveraging() {
    double id[] = {1, 2, 3, 4, 5, 6};
    std::vector<double> inputData(id, id + sizeof(id) / sizeof(double));
    double ib[] = {0, 1, 2, 3, 4, 5};
    std::vector<double> inputBoundaries(ib, ib + sizeof(ib) / sizeof(double));

    std::vector<double> output;
    TS_ASSERT_THROWS(VectorHelper::smoothInRange(inputData, output, 6, &inputBoundaries),
                     const std::invalid_argument &);
    inputBoundaries.emplace_back(6);
    VectorHelper::smoothInRange(inputData, output, 6, &inputBoundaries);

    TS_ASSERT_DELTA(output[1] - output[0], 0.492, 1.e-3);
    TS_ASSERT_DELTA(output[3] - output[2], 0.4545, 1.e-3);
    TS_ASSERT_DELTA(output[5] - output[4], 0.492, 1.e-3);
    inputBoundaries[1] = 1;
    inputBoundaries[2] = 3;
    inputBoundaries[3] = 6;
    inputBoundaries[4] = 10;
    inputBoundaries[5] = 15;
    inputBoundaries[6] = 21;
    VectorHelper::smoothInRange(inputData, output, 6, &inputBoundaries);
    TS_ASSERT_DELTA(output[2], 3, 1.e-8);
    TS_ASSERT_DELTA(output[0], 1, 1.e-8);
    TS_ASSERT_DELTA(output[5], 6, 1.e-8);

    std::vector<double> out_bins;
    VectorHelper::smoothInRange(inputData, output, 3, &inputBoundaries, 1, 5, &out_bins);
    TS_ASSERT_EQUALS(output.size(), 4);
    TS_ASSERT_DELTA(output[1], 3, 1.e-8);
  }

  void test_Smooth_keeps_peakPosition() {

    std::vector<double> output;
    std::vector<double> inputBoundaries(21);
    inputBoundaries[0] = 0;
    double step(1);
    for (size_t i = 1; i < 21; i++) {
      inputBoundaries[i] = inputBoundaries[i - 1] + step;
      step *= 1.1;
    }
    double norm = 100 / inputBoundaries[20];
    for (size_t i = 0; i < 21; i++) {
      inputBoundaries[i] *= norm;
    }

    std::vector<double> inputData(20);
    for (size_t i = 0; i < 20; i++) {
      double dev = 0.5 * (inputBoundaries[i] + inputBoundaries[i + 1]) - 50;
      inputData[i] = exp(-dev * dev / 100) * (inputBoundaries[i + 1] - inputBoundaries[i]);
    }
    int indOfMax = VectorHelper::getBinIndex(inputBoundaries, 50.);
    double fMax = inputData[indOfMax] / (inputBoundaries[indOfMax + 1] - inputBoundaries[indOfMax]);
    double iLeft = inputData[indOfMax - 1] / (inputBoundaries[indOfMax] - inputBoundaries[indOfMax - 1]);
    double iRight = inputData[indOfMax + 1] / (inputBoundaries[indOfMax + 2] - inputBoundaries[indOfMax + 1]);

    TS_ASSERT(iLeft < fMax);
    TS_ASSERT(iRight < fMax);
    VectorHelper::smoothInRange(inputData, output, 10, &inputBoundaries);
    fMax = output[indOfMax] / (inputBoundaries[indOfMax + 1] - inputBoundaries[indOfMax]);
    iLeft = inputData[indOfMax - 1] / (inputBoundaries[indOfMax] - inputBoundaries[indOfMax - 1]);
    iRight = inputData[indOfMax + 1] / (inputBoundaries[indOfMax + 2] - inputBoundaries[indOfMax + 1]);

    TS_ASSERT(iLeft < fMax);
    TS_ASSERT(iRight < fMax);

    output.swap(inputData);
    VectorHelper::smoothInRange(inputData, output, 10, &inputBoundaries);

    fMax = output[indOfMax] / (inputBoundaries[indOfMax + 1] - inputBoundaries[indOfMax]);
    iLeft = inputData[indOfMax - 1] / (inputBoundaries[indOfMax] - inputBoundaries[indOfMax - 1]);
    iRight = inputData[indOfMax + 1] / (inputBoundaries[indOfMax + 2] - inputBoundaries[indOfMax + 1]);

    //  TS_ASSERT(iLeft<fMax);
    TS_ASSERT(iRight < fMax);

    output.swap(inputData);
    VectorHelper::smoothInRange(inputData, output, 10, &inputBoundaries);

    fMax = output[indOfMax] / (inputBoundaries[indOfMax + 1] - inputBoundaries[indOfMax]);
    iLeft = inputData[indOfMax - 1] / (inputBoundaries[indOfMax] - inputBoundaries[indOfMax - 1]);
    iRight = inputData[indOfMax + 1] / (inputBoundaries[indOfMax + 2] - inputBoundaries[indOfMax + 1]);

    // TS_ASSERT(iLeft<fMax);
    TS_ASSERT(iRight < fMax);

    output.swap(inputData);
    VectorHelper::smoothInRange(inputData, output, 10, &inputBoundaries);

    fMax = output[indOfMax] / (inputBoundaries[indOfMax + 1] - inputBoundaries[indOfMax]);
    iLeft = inputData[indOfMax - 1] / (inputBoundaries[indOfMax] - inputBoundaries[indOfMax - 1]);
    iRight = inputData[indOfMax + 1] / (inputBoundaries[indOfMax + 2] - inputBoundaries[indOfMax + 1]);

    TS_ASSERT(inputData[indOfMax - 1] < output[indOfMax]);
    TS_ASSERT(inputData[indOfMax + 1] < output[indOfMax]);
  }

private:
  /// Testing bins
  std::vector<double> m_test_bins;
};

class VectorHelperTestPerformance : public CxxTest::TestSuite {
public:
  VectorHelperTestPerformance *createSuite() { return new VectorHelperTestPerformance(); }
  void destroySuite(VectorHelperTestPerformance *suite) { delete suite; }

  VectorHelperTestPerformance() {
    setupHistogram();
    setupOutput();
  }

  void testRebinSmaller() {
    auto size = smallerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      VectorHelper::rebin(binEdges, counts, errors, smallerBinEdges, yout, eout, false, false);
    }
  }

  void testRebinSmallerFrequencies() {
    auto size = smallerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      VectorHelper::rebin(binEdges, frequencies, frequencyErrors, smallerBinEdges, yout, eout, true, false);
    }
  }

  void testRebinLarger() {
    auto size = largerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      VectorHelper::rebin(binEdges, counts, errors, largerBinEdges, yout, eout, false, false);
    }
  }

  void testRebinLargerFrequencies() {
    auto size = largerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      VectorHelper::rebin(binEdges, frequencies, frequencyErrors, largerBinEdges, yout, eout, true, false);
    }
  }

private:
  const size_t binSize = 10000;
  const size_t nIters = 10000;
  std::vector<double> binEdges;
  std::vector<double> counts;
  std::vector<double> frequencies;
  std::vector<double> errors;
  std::vector<double> frequencyErrors;
  std::vector<double> smallerBinEdges;
  std::vector<double> largerBinEdges;

  void setupHistogram() {
    binEdges.resize(binSize);
    frequencies.resize(binSize - 1);
    frequencyErrors.resize(binSize - 1);
    counts.resize(binSize - 1);
    errors.resize(binSize - 1);

    std::iota(binEdges.begin(), binEdges.end(), 0);
    std::generate(counts.begin(), counts.end(), []() { return static_cast<double>(rand() % 1000); });

    for (size_t i = 0; i < counts.size(); i++)
      frequencies[i] = counts[i] / (binEdges[i + 1] - binEdges[i]);

    std::transform(counts.cbegin(), counts.cend(), errors.begin(), [](const double count) { return sqrt(count); });

    std::transform(frequencies.cbegin(), frequencies.cend(), frequencyErrors.begin(),
                   [](const double freq) { return sqrt(freq); });
  }

  void setupOutput() {
    smallerBinEdges.resize(binSize * 2);
    largerBinEdges.resize(binSize / 2);

    auto binWidth = binEdges[1] - binEdges[0];

    for (size_t i = 0; i < binSize - 1; i++) {
      smallerBinEdges[2 * i] = binEdges[i];
      smallerBinEdges[(2 * i) + 1] = (binEdges[i] + binEdges[i + 1]) / 2;
    }
    smallerBinEdges[2 * (binSize - 1)] = binEdges.back();
    smallerBinEdges.back() = binEdges.back() + (binWidth / 2);

    for (size_t i = 0; i < largerBinEdges.size(); i++)
      largerBinEdges[i] = binEdges[(2 * i)];
  }
};

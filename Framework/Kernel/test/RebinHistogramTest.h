#ifndef REBINHISTOGRAM_TEST_H_
#define REBINHISTOGRAM_TEST_H_

#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <numeric>
#include <vector>

/// @author Laurent C Chapon, ISIS Facility, Rutherford Appleton Laboratory
/// 13/03/2009
/// This is testing the validity of the rebinHistogram function in vectorHelper
/// Class

class RebinHistogramTest : public CxxTest::TestSuite {
public:
  /// Create a new X vector where the steps are half the size of the old one.
  /// Perform rebin and check the values
  /// Y data should now contains half the intensity
  /// E data should contains the
  /// Perform another rebin in the opposite direction and check that the data
  /// are identical to initial values
  void TestRebinSmallerSteps() {
    // Size of vectors
    size_t size1 = 12, size2 = 23;
    std::vector<double> xin(size1);
    std::vector<double> yin(size1 - 1);
    std::vector<double> ein(size1 - 1);
    std::vector<double> xout(size2);
    std::vector<double> yout(size2 - 1);
    std::vector<double> eout(size2 - 1);
    for (std::size_t i = 0; i < size1 - 1; i++) {
      xin[i] = (double)(i);
      yin[i] = 1.0;
      ein[i] = 1.0;
    }
    xin[size1 - 1] = static_cast<double>(size1 - 1);
    for (std::size_t i = 0; i < size2; i++)
      xout[i] = 0.5 * static_cast<double>(i);
    Mantid::Kernel::VectorHelper::rebinHistogram(xin, yin, ein, xout, yout,
                                                 eout, false);
    for (std::size_t i = 0; i < size2 - 1; i++) {
      TS_ASSERT_DELTA(yout[i], 0.5, 1e-7);
      TS_ASSERT_DELTA(eout[i], M_SQRT1_2, 1e-7);
    }
    std::vector<double> returnX(xin), returnY(size1 - 1), returnE(size1 - 1);

    Mantid::Kernel::VectorHelper::rebinHistogram(xout, yout, eout, returnX,
                                                 returnY, returnE, false);
    for (std::size_t i = 0; i < size1 - 1; i++) {
      TS_ASSERT_DELTA(returnY[i], yin[i], 1e-7);
      TS_ASSERT_DELTA(returnE[i], ein[i], 1e-7);
    }
  }
};

class RebinHistogramTestPerformance : public CxxTest::TestSuite {
public:
  RebinHistogramTestPerformance *createSuite() {
    return new RebinHistogramTestPerformance();
  }

  void destroySuite(RebinHistogramTestPerformance *suite) { delete suite; }

  RebinHistogramTestPerformance() {
    setupHistogram();
    setupOutput();
  }

  void testRebinSmaller() {
    auto size = smallerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      Mantid::Kernel::VectorHelper::rebinHistogram(
          binEdges, counts, errors, smallerBinEdges, yout, eout, false);
    }
  }

  void testRebinLarger() {
    auto size = largerBinEdges.size() - 1;
    for (size_t i = 0; i < nIters; i++) {
      std::vector<double> yout(size);
      std::vector<double> eout(size);
      Mantid::Kernel::VectorHelper::rebinHistogram(
          binEdges, counts, errors, largerBinEdges, yout, eout, false);
    }
  }

private:
  const size_t binSize = 10000;
  const size_t nIters = 10000;
  std::vector<double> binEdges;
  std::vector<double> counts;
  std::vector<double> errors;
  std::vector<double> smallerBinEdges;
  std::vector<double> largerBinEdges;

  void setupHistogram() {
    binEdges.resize(binSize);
    counts.resize(binSize - 1);
    errors.resize(binSize - 1);

    std::iota(binEdges.begin(), binEdges.end(), 0);
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

#endif // REBINHISTOGRAM_TEST_H_/

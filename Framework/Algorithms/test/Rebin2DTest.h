// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REBIN2DTEST_H_
#define MANTID_ALGORITHMS_REBIN2DTEST_H_

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/Timer.h"

using Mantid::Algorithms::Rebin2D;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
/**
 * Shared methods between the unit test and performance test
 */

/// Return the input workspace. All Y values are 2 and E values sqrt(2)
MatrixWorkspace_sptr makeInputWS(const bool distribution,
                                 const bool perf_test = false,
                                 const bool small_bins = false) {
  size_t nhist(0), nbins(0);
  double x0(0.0), deltax(0.0);

  if (perf_test) {
    nhist = 500;
    nbins = 400;
    x0 = 100.;
    deltax = 100.;
  } else {
    nhist = 10;
    nbins = 10;
    x0 = 5.0;
    if (small_bins) {
      deltax = 0.1;
    } else {
      deltax = distribution ? 2.0 : 1.0;
    }
  }

  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(
      int(nhist), int(nbins), x0, deltax);

  // We need something other than a spectrum axis, call this one theta
  auto thetaAxis = std::make_unique<BinEdgeAxis>(nhist + 1);
  for (size_t i = 0; i < nhist + 1; ++i) {
    thetaAxis->setValue(i, -0.5 + static_cast<double>(i));
  }
  ws->replaceAxis(1, std::move(thetaAxis));

  if (distribution) {
    Mantid::API::WorkspaceHelpers::makeDistribution(ws);
  }

  return ws;
}

MatrixWorkspace_sptr runAlgorithm(MatrixWorkspace_sptr inputWS,
                                  const std::string &axis1Params,
                                  const std::string &axis2Params,
                                  const bool UseFractionalArea = false) {
  // Name of the output workspace.
  std::string outWSName("Rebin2DTest_OutputWS");

  Rebin2D alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize())
  TS_ASSERT(alg.isInitialized())
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis1Binning", axis1Params));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis2Binning", axis2Params));
  TS_ASSERT_THROWS_NOTHING(
      alg.setProperty("UseFractionalArea", UseFractionalArea));
  TS_ASSERT_THROWS_NOTHING(alg.execute(););
  TS_ASSERT(alg.isExecuted());

  MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(outWSName));
  TS_ASSERT(outputWS);
  return outputWS;
}
} // namespace

class Rebin2DTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static Rebin2DTest *createSuite() { return new Rebin2DTest(); }
  static void destroySuite(Rebin2DTest *suite) { delete suite; }

  void test_Init() {
    Rebin2D alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Rebin2D_With_Axis2_Unchanged() {
    MatrixWorkspace_sptr inputWS = makeInputWS(false); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,2.,15.", "-0.5,1,9.5");

    checkData(outputWS, 6, 10, false, true);
  }

  void test_Rebin2D_With_Axis1_Unchanged() {
    MatrixWorkspace_sptr inputWS = makeInputWS(false); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,1.,15.", "-0.5,2,9.5");
    checkData(outputWS, 11, 5, false, false);
  }

  void test_Rebin2D_With_Input_Distribution() {
    MatrixWorkspace_sptr inputWS = makeInputWS(true); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,4.,25.", "-0.5,1,9.5");
    checkData(outputWS, 6, 10, true, true);
  }

  void test_Rebin2D_With_Bin_Width_Less_Than_One_And_Not_Distribution() {
    MatrixWorkspace_sptr inputWS =
        makeInputWS(false, false, true); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,0.2,6.", "-0.5,1,9.5");
    checkData(outputWS, 6, 10, false, true, true);
  }

  void test_BothAxes() {
    // 5,6,7,8,9,10,11,12,12,14,15
    MatrixWorkspace_sptr inputWS =
        makeInputWS(false, false, false); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,1.8,15", "-0.5,2.5,9.5");
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 6);

    double errors[6] = {3., 3., 3., 3., 3., 2.236067977};

    const double epsilon(1e-08);
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &y = outputWS->y(i);
      const auto &e = outputWS->e(i);
      const size_t numBins = y.size();
      for (size_t j = 0; j < numBins; ++j) {
        if (j < 5) {
          TS_ASSERT_DELTA(y[j], 9, epsilon);
        } else {
          // Last bin
          TS_ASSERT_DELTA(y[j], 5, epsilon);
        }
        TS_ASSERT_DELTA(e[j], errors[j], epsilon);
      }
    }
  }

  void test_BothAxes_FractionalArea() {
    MatrixWorkspace_sptr inputWS =
        makeInputWS(false, false, false); // 10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS =
        runAlgorithm(inputWS, "5.,1.8,15", "-0.5,2.5,9.5", true);
    TS_ASSERT_EQUALS(outputWS->id(), "RebinnedOutput");
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 6);

    const double epsilon(1e-08);
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &y = outputWS->y(i);
      const auto &e = outputWS->e(i);
      const size_t numBins = y.size();
      for (size_t j = 0; j < numBins; ++j) {
        TS_ASSERT_DELTA(y[j], 2, epsilon);
        if (j < 5) {
          TS_ASSERT_DELTA(e[j], 2. / 3., epsilon);
        } else {
          // Last bin
          TS_ASSERT_DELTA(e[j], sqrt(0.8), epsilon);
        }
      }
    }
  }

  void test_Zero_Area_Bins_NoFractionalBinning() {
    MatrixWorkspace_sptr inputWS = makeInputWS(false);
    const auto nhist = inputWS->getNumberHistograms();
    // Set the vertical 'width' of a single histogram to zero
    auto thetaAxis = inputWS->getAxis(1);
    const auto middle = nhist / 2;
    const auto midValue = thetaAxis->getValue(middle);
    thetaAxis->setValue(middle - 1, midValue);
    constexpr bool useFractionalBinning = false;
    MatrixWorkspace_sptr outputWS = runAlgorithm(
        inputWS, "5.,2.,15.", "-0.5,10.,9.5", useFractionalBinning);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto expectedY = 2. * 9. * 2.;
    const auto expectedE = std::sqrt(expectedY);
    const auto &Ys = outputWS->y(0);
    const auto &Es = outputWS->e(0);
    for (size_t i = 0; i < Ys.size(); ++i) {
      TS_ASSERT(!std::isnan(Ys[i]))
      TS_ASSERT_DELTA(Ys[i], expectedY, 1e-12)
      TS_ASSERT_DELTA(Es[i], expectedE, 1e-12)
    }
  }

  void test_Zero_Area_Bins_FractionalBinning() {
    MatrixWorkspace_sptr inputWS = makeInputWS(false);
    const auto nhist = inputWS->getNumberHistograms();
    // Set the vertical 'width' of a single histogram to zero
    auto thetaAxis = inputWS->getAxis(1);
    const auto middle = nhist / 2;
    const auto midValue = thetaAxis->getValue(middle);
    thetaAxis->setValue(middle - 1, midValue);
    constexpr bool useFractionalBinning = true;
    MatrixWorkspace_sptr outputWS = runAlgorithm(
        inputWS, "5.,2.,15.", "-0.5,10.,9.5", useFractionalBinning);
    const auto &rebinned = *dynamic_cast<RebinnedOutput *>(outputWS.get());
    TS_ASSERT_EQUALS(rebinned.getNumberHistograms(), 1)
    const auto expectedY = 2. * 9. * 2.;
    const auto expectedE = std::sqrt(expectedY);
    const auto &Fs = rebinned.dataF(0);
    const auto &Ys = rebinned.y(0);
    const auto &Es = rebinned.e(0);
    for (size_t i = 0; i < Ys.size(); ++i) {
      TS_ASSERT(!std::isnan(Ys[i]))
      TS_ASSERT_DELTA(Ys[i] * Fs[i], expectedY, 1e-12)
      TS_ASSERT_DELTA(Es[i] * Fs[i], expectedE, 1e-12)
    }
  }

private:
  void checkData(MatrixWorkspace_const_sptr outputWS, const size_t nxvalues,
                 const size_t nhist, const bool dist, const bool onAxis1,
                 const bool small_bins = false) {
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nhist);
    TS_ASSERT_EQUALS(outputWS->isDistribution(), dist);
    // Axis sizes
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), nhist + 1);
    TS_ASSERT_EQUALS(outputWS->x(0).size(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->y(0).size(), nxvalues - 1);

    const double epsilon(1e-08);
    for (size_t i = 0; i < nhist; ++i) {
      const auto &x = outputWS->x(i);
      const auto &y = outputWS->y(i);
      const auto &e = outputWS->e(i);
      for (size_t j = 0; j < nxvalues - 1; ++j) {
        if (onAxis1) {
          if (small_bins) {

            TS_ASSERT_DELTA(x[j], 5.0 + 0.2 * static_cast<double>(j), epsilon);
          } else {
            if (dist) {
              TS_ASSERT_DELTA(x[j], 5.0 + 4.0 * static_cast<double>(j),
                              epsilon);
            } else {
              TS_ASSERT_DELTA(x[j], 5.0 + 2.0 * static_cast<double>(j),
                              epsilon);
            }
          }
        } else {
          TS_ASSERT_DELTA(x[j], 5.0 + static_cast<double>(j), epsilon);
        }
        if (dist) {
          TS_ASSERT_DELTA(y[j], 1.0, epsilon);
          TS_ASSERT_DELTA(e[j], 0.5, epsilon);
        } else {
          std::ostringstream os;
          os << "Bin " << i << "," << j;
          TSM_ASSERT_DELTA(os.str(), y[j], 4.0, epsilon);
          TS_ASSERT_DELTA(e[j], 2.0, epsilon);
        }
      }
      // Final X boundary
      if (small_bins) {
        TS_ASSERT_DELTA(x[nxvalues - 1], 6.0, epsilon);
      } else {
        if (dist) {
          TS_ASSERT_DELTA(x[nxvalues - 1], 25.0, epsilon);
        } else {
          TS_ASSERT_DELTA(x[nxvalues - 1], 15.0, epsilon);
        }
      }
    }
    // Clean up
    AnalysisDataService::Instance().remove(outputWS->getName());
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class Rebin2DTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Rebin2DTestPerformance *createSuite() {
    return new Rebin2DTestPerformance();
  }
  static void destroySuite(Rebin2DTestPerformance *suite) { delete suite; }

  Rebin2DTestPerformance() {
    constexpr bool distribution = false;
    constexpr bool perf_test = true;
    constexpr bool small_bins = false;
    m_inputWS = makeInputWS(distribution, perf_test, small_bins);
  }

  void test_On_Large_Workspace() {
    runAlgorithm(m_inputWS, "100,10,41000", "-0.5,0.5,499.5");
  }

  void test_Use_Fractional_Area() {
    constexpr bool useFractionalArea = true;
    runAlgorithm(m_inputWS, "100,10,41000", "-0.5,0.5,499.5",
                 useFractionalArea);
  }

private:
  MatrixWorkspace_sptr m_inputWS;
};

#endif /* MANTID_ALGORITHMS_REBIN2DTEST_H_ */

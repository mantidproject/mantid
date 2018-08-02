#ifndef MANTID_ALGORITHMS_REBIN2DTEST_H_
#define MANTID_ALGORITHMS_REBIN2DTEST_H_

#include "MantidAPI/NumericAxis.h"
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/Timer.h"

using Mantid::Algorithms::Rebin2D;
using namespace Mantid::API;

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
  NumericAxis *const thetaAxis = new NumericAxis(nhist + 1);
  for (size_t i = 0; i < nhist + 1; ++i) {
    thetaAxis->setValue(i, -0.5 + static_cast<double>(i));
  }
  ws->replaceAxis(1, thetaAxis);

  if (distribution) {
    Mantid::API::WorkspaceHelpers::makeDistribution(ws);
  }

  return ws;
}

MatrixWorkspace_sptr runAlgorithm(MatrixWorkspace_sptr inputWS,
                                  const std::string &axis1Params,
                                  const std::string &axis2Params) {
  // Name of the output workspace.
  std::string outWSName("Rebin2DTest_OutputWS");

  Rebin2D alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize())
  TS_ASSERT(alg.isInitialized())
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis1Binning", axis1Params));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis2Binning", axis2Params));
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
        std::ostringstream os;
        os << "Bin " << i << "," << j;
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
  Rebin2DTestPerformance() {
    m_inputWS = makeInputWS(distribution, perf_test, small_bins);
  }

  void test_On_Large_Workspace() {
    runAlgorithm(m_inputWS, "100,200,41000", "-0.5,2,499.5");
  }

private:
  MatrixWorkspace_sptr m_inputWS;

  const bool distribution = false;
  const bool perf_test = true;
  const bool small_bins = false;
};

#endif /* MANTID_ALGORITHMS_REBIN2DTEST_H_ */

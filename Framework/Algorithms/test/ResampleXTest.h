#ifndef MANTID_ALGORITHMS_REBINRAGGEDTEST_H_
#define MANTID_ALGORITHMS_REBINRAGGEDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ResampleX.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::Algorithms::ResampleX;
using Mantid::MantidVec;

class ResampleXTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResampleXTest *createSuite() { return new ResampleXTest(); }
  static void destroySuite(ResampleXTest *suite) { delete suite; }

  void test_Init() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_linear_binning_histogram() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int numBins(3000);
    MantidVec xValues;
    double delta;

    // testing linear binning for histogram
    alg.setOptions(numBins, false, false);
    delta = alg.determineBinning(xValues, 0., 300.);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size() - 1);
    TS_ASSERT_DELTA(0.1, delta, .001);
    TS_ASSERT_EQUALS(0.0, xValues[0]);
    TS_ASSERT_EQUALS(0.1, xValues[1]);
    TS_ASSERT_EQUALS(300., xValues[3000]);
  }

  void test_linear_binning_density() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int numBins(3000);
    MantidVec xValues;
    double delta;

    // testing linear binning for density
    alg.setOptions(numBins, false, true);
    delta = alg.determineBinning(xValues, 0.1, 300);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size());
    TS_ASSERT_DELTA(0.1, delta, .001);
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(0.2, xValues[1]);
    TS_ASSERT_EQUALS(300., xValues[2999]);
  }

  void test_log_binning_histogram() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    //
    int numBins(3000);
    MantidVec xValues;
    double delta;

    alg.setOptions(numBins, true, false);

    // first check that using zero for a border doesn't work
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, 0, 300));
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, -300, 0));

    // do an actual run
    delta = alg.determineBinning(xValues, 0.1, 1.0);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size() - 1);
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(1., xValues[3000]);
    TS_ASSERT_DELTA(-.00077, delta, .00001);
  }

  void test_log_binning_density() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    //
    int numBins(3000);
    MantidVec xValues;
    double delta;

    alg.setOptions(numBins, true, true);

    // first check that using zero for a border doesn't work
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, 0, 300));
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, -300, 0));

    // do an actual run
    delta = alg.determineBinning(xValues, 0.1, 1.0);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size());
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(1., xValues[2999]);
    TS_ASSERT_DELTA(-.00077, delta, .00001);
  }

  void xtest_exec() {
    std::string outWSName("ResampleX_out");

    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "value"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    //    "XMin"
    //    "XMax"
    //    "NumberBins"
    //    "LogBinning"
    //    "PreserveEvents"
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_ALGORITHMS_REBINRAGGEDTEST_H_ */

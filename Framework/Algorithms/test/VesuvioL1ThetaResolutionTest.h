#ifndef MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_
#define MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/VesuvioL1ThetaResolution.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"

using Mantid::Algorithms::VesuvioL1ThetaResolution;
using namespace Mantid::API;

class VesuvioL1ThetaResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VesuvioL1ThetaResolutionTest *createSuite() {
    return new VesuvioL1ThetaResolutionTest();
  }
  static void destroySuite(VesuvioL1ThetaResolutionTest *suite) {
    delete suite;
  }

  void test_Init() {
    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /**
   * Tests execution with default options and no PAR file.
   */
  void test_runDefaultOptions() {
    // Name of the output workspace.
    std::string outWSName("VesuvioL1ThetaResolutionTest_OutputWS");

    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", 100));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;
    validateFullResolutionWorkspace(ws);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
   * Tests execution with a PAR file.
   */
  void test_runPARFile() {
    // Name of the output workspace.
    std::string outWSName("VesuvioL1ThetaResolutionTest_OutputWS");

    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", 100));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PARFile", "IP0005.dat"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;
    validateFullResolutionWorkspace(ws);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
   * Tests execution outputting the L1 distribution workspace.
   */
  void test_runL1Distribution() {
    // Name of the output workspace.
    std::string outWSName("VesuvioL1ThetaResolutionTest_OutputWS");
    std::string l1WSName("VesuvioL1ThetaResolutionTest_OutputWS_L1");

    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("L1Distribution", l1WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", 100));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            l1WSName));
    TS_ASSERT(ws);
    if (!ws)
      return;
    validateFullDistributionWorkspace(ws, "l1");

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
   * Tests execution outputting the theta distribution workspace.
   */
  void test_runThetaDistribution() {
    // Name of the output workspace.
    std::string outWSName("VesuvioL1ThetaResolutionTest_OutputWS");
    std::string thetaWSName("VesuvioL1ThetaResolutionTest_OutputWS_Theta");

    VesuvioL1ThetaResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ThetaDistribution", thetaWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumEvents", 100));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            thetaWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;
    validateFullDistributionWorkspace(ws, "theta");

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

private:
  void validateFullResolutionWorkspace(MatrixWorkspace_sptr ws) {
    NumericAxis *xAxis = dynamic_cast<NumericAxis *>(ws->getAxis(0));
    TS_ASSERT(xAxis);
    if (xAxis) {
      TS_ASSERT_EQUALS(xAxis->length(), 196);
      TS_ASSERT_EQUALS(xAxis->getValue(0), 3);
      TS_ASSERT_EQUALS(xAxis->getValue(xAxis->length() - 1), 198);
      TS_ASSERT_EQUALS(xAxis->unit()->unitID(), "Label");
      TS_ASSERT_EQUALS(xAxis->unit()->caption(), "Spectrum Number");
    }

    TextAxis *vAxis = dynamic_cast<TextAxis *>(ws->getAxis(1));
    TS_ASSERT(vAxis);
    if (vAxis) {
      TS_ASSERT_EQUALS(vAxis->length(), 4);
    }
  }

  void validateFullDistributionWorkspace(MatrixWorkspace_sptr ws,
                                         const std::string &label) {
    NumericAxis *xAxis = dynamic_cast<NumericAxis *>(ws->getAxis(0));
    TS_ASSERT(xAxis);
    if (xAxis) {
      TS_ASSERT_EQUALS(xAxis->unit()->unitID(), "Label");
      TS_ASSERT_EQUALS(xAxis->unit()->caption(), label);
    }

    SpectraAxis *vAxis = dynamic_cast<SpectraAxis *>(ws->getAxis(1));
    TS_ASSERT(vAxis);
    if (vAxis) {
      TS_ASSERT_EQUALS(vAxis->length(), 196);
      TS_ASSERT_EQUALS(vAxis->getValue(0), 3);
      TS_ASSERT_EQUALS(vAxis->getValue(vAxis->length() - 1), 198);
    }

    auto firstSpecDetIDs = ws->getSpectrum(0).getDetectorIDs();
    TS_ASSERT_EQUALS(firstSpecDetIDs.size(), 1);
    TS_ASSERT_DIFFERS(firstSpecDetIDs.find(2101), firstSpecDetIDs.end());

    auto lastSpecDetIDs =
        ws->getSpectrum(ws->getNumberHistograms() - 1).getDetectorIDs();
    TS_ASSERT_EQUALS(lastSpecDetIDs.size(), 1);
    TS_ASSERT_DIFFERS(lastSpecDetIDs.find(3232), lastSpecDetIDs.end());
  }
};

#endif /* MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTIONTEST_H_ */

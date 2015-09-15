#ifndef MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_
#define MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidAlgorithms/ConvertEmptyToTof.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

using Mantid::Algorithms::ConvertEmptyToTof;
using namespace Mantid;
using namespace API;

class ConvertEmptyToTofTest: public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertEmptyToTofTest *createSuite() {
    return new ConvertEmptyToTofTest();
  }
  static void destroySuite(ConvertEmptyToTofTest *suite) {
    delete suite;
  }

  ConvertEmptyToTofTest() {
    FrameworkManager::Instance();
  }

  void test_Init() {
    ConvertEmptyToTof alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_find_ep_from_1_spectra() {
    // Name of the output workspace.
    std::string outWSName("ConvertEmptyToTofTest_OutputWS1");
    std::string inWSName("ConvertEmptyToTofTest_InputWS1");

    DataObjects::Workspace2D_sptr testWS = createTestWorkspace();
    WorkspaceCreationHelper::storeWS(inWSName, testWS);

    ConvertEmptyToTof alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ListOfSpectraIndices", "5"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ListOfChannelIndices", "40-60"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(); );
    TS_ASSERT(alg.isExecuted());

    auto outWS =
        AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            outWSName);

    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Check the results
    TS_ASSERT_DELTA(*(outWS->dataX(1).begin()), 31463.8, 0.1);
    TS_ASSERT_DELTA(*(outWS->dataX(1).end() - 1), 34493.8, 0.1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_find_ep_from_2_spectra() {
    // Name of the output workspace.
    std::string outWSName("ConvertEmptyToTofTest_OutputWS2");
    std::string inWSName("ConvertEmptyToTofTest_InputWS2");

    DataObjects::Workspace2D_sptr testWS = createTestWorkspace();
    WorkspaceCreationHelper::storeWS(inWSName, testWS);

    // move
    placeDetectorAtSamePosition(testWS, "pixel-5)", "pixel-6)");

    ConvertEmptyToTof alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ListOfSpectraIndices", "5,6"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ListOfChannelIndices", "40-60"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(); );
    TS_ASSERT(alg.isExecuted());

    auto outWS =
        AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            outWSName);

    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Check the results
    TS_ASSERT_DELTA(*(outWS->dataX(1).begin()), 31433.8, 0.1);
    TS_ASSERT_DELTA(*(outWS->dataX(1).end() - 1), 34463.8, 0.1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_set_tof_from_EPP_and_EP_Spectrum_idx() {
      // Name of the output workspace.
      std::string outWSName("ConvertEmptyToTofTest_OutputWS3");
      std::string inWSName("ConvertEmptyToTofTest_InputWS3");

      DataObjects::Workspace2D_sptr testWS = createTestWorkspace();
      WorkspaceCreationHelper::storeWS(inWSName, testWS);

      ConvertEmptyToTof alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));

      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ElasticPeakPositionSpectrum", "5"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ElasticPeakPosition", "50"));
      TS_ASSERT_THROWS_NOTHING(alg.execute(); );
      TS_ASSERT(alg.isExecuted());

      auto outWS =
          AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
              outWSName);

      TS_ASSERT(outWS);
      if (!outWS)
        return;

      // Check the results
      TS_ASSERT_DELTA(*(outWS->dataX(1).begin()), 30113.8, 0.1);
      TS_ASSERT_DELTA(*(outWS->dataX(1).end() - 1), 33143.8, 0.1);

      // Remove workspace from the data service.
      AnalysisDataService::Instance().remove(outWSName);
    }

private:

  /**
   * Create a test workspace with full instrument
   * Spectra follow a Gaussian distribution
   */
  DataObjects::Workspace2D_sptr createTestWorkspace() {

    // create test ws
    const size_t nHist = 10; //or testWS->getNumberHistograms();
    const size_t nBins = 101; //or testWS->blocksize();
    DataObjects::Workspace2D_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nHist,
            nBins, false, false, true, std::string("testInstEmpty"));
    testWS->getAxis(0)->setUnit("Empty");
    API::Run & run = testWS->mutableRun();
    run.addProperty<double>("wavelength", 5.0, true); //overwrite
    run.addProperty<double>("channel_width", 30.0, true); //overwrite

    for (size_t i = 0; i < nHist; ++i) {
      for (size_t j = 0; j < nBins - 1; ++j) {
        // gaussian peak centred at 50,and h=10
        testWS->dataY(i)[j] = 10
            * exp(-pow((static_cast<double>(j) - 50), 2) / (2 * pow(1.5, 2)));
      }
    }
    return testWS;
  }
  /**
   * Place componentName1 and componentName2 at the same distance!
   */
  void placeDetectorAtSamePosition(API::MatrixWorkspace_sptr ws,
      const std::string& componentName1, const std::string& componentName2) {

    Geometry::Instrument_const_sptr instrument = ws->getInstrument();
    Geometry::IComponent_const_sptr component1 = instrument->getComponentByName(
        componentName1);
    Geometry::IComponent_const_sptr component2 = instrument->getComponentByName(
        componentName2);

    if (component1 == 0 || component2 == 0)
      throw std::runtime_error(
          "component1 = 0 || component2 == 0 : Not found!");

    Kernel::V3D component1Pos = component1->getPos();
    Kernel::V3D component2NewPos(component1Pos);

    Geometry::ParameterMap& pmap = ws->instrumentParameters();
    Geometry::ComponentHelper::moveComponent(*component2, pmap,
        component2NewPos, Geometry::ComponentHelper::Absolute);

  }

};

#endif /* MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_ */

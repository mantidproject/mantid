#ifndef MULTIPLE_SCATTERING_ABSORPTION_TEST_H_
#define MULTIPLE_SCATTERING_ABSORPTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAlgorithms/MultipleScatteringCylinderAbsorption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::MantidVec;

class MultipleScatteringCylinderAbsorptionTest : public CxxTest::TestSuite {
public:
  void testName() {
    TS_ASSERT_EQUALS(algorithm.name(), "MultipleScatteringCylinderAbsorption");
  }

  void testVersion() { TS_ASSERT_EQUALS(algorithm.version(), 1); }

  void testInit() {
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption algorithm_b;
    TS_ASSERT_THROWS_NOTHING(algorithm_b.initialize());
    TS_ASSERT(algorithm_b.isInitialized());

    const std::vector<Property *> props = algorithm_b.getProperties();
    TS_ASSERT_EQUALS(props.size(), 6);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "AttenuationXSection");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), "ScatteringXSection");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[3]));

    TS_ASSERT_EQUALS(props[4]->name(), "SampleNumberDensity");
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[4]));

    TS_ASSERT_EQUALS(props[5]->name(), "CylinderSampleRadius");
    TS_ASSERT(props[5]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[5]));
  }

  void testCalculationHist() {
    // setup the test workspace
    Workspace2D_sptr wksp =
        WorkspaceCreationHelper::create2DWorkspaceBinned(9, 16, 1000, 1000);
    wksp->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    wksp->getAxis(0)->setUnit("TOF");
    AnalysisDataService::Instance().add("TestInputWS", wksp);

    // convert to wavelength
    auto convertUnitsAlg =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "ConvertUnits");
    convertUnitsAlg->setPropertyValue("InputWorkspace", "TestInputWS");
    convertUnitsAlg->setPropertyValue("OutputWorkspace", "TestInputWS");
    convertUnitsAlg->setProperty("Target", "Wavelength");
    convertUnitsAlg->execute();

    // create and execute the algorithm
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption algorithm_c;
    TS_ASSERT_THROWS_NOTHING(algorithm_c.initialize());
    TS_ASSERT(algorithm_c.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("InputWorkspace", "TestInputWS"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("OutputWorkspace", "TestOutputWS"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("CylinderSampleRadius", "0.3175"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("AttenuationXSection", "2.8"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("SampleNumberDensity", "0.0721"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm_c.setPropertyValue("ScatteringXSection", "5.1"));

    TS_ASSERT_THROWS_NOTHING(algorithm_c.execute());
    TS_ASSERT(algorithm_c.isExecuted());

    MatrixWorkspace_sptr test_output_WS;

    TS_ASSERT_THROWS_NOTHING(
        test_output_WS =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                "TestOutputWS"));
    TS_ASSERT(test_output_WS);

    // setup expected values
    const size_t size = 16;
    std::array<double, size> y_expected = {
        {2.22389, 2.2924, 2.36292, 2.43552, 2.51024, 2.58716, 2.66632, 2.7478,
         2.83166, 2.91796, 3.00678, 3.0982, 3.19228, 3.28912, 3.38879,
         3.49139}};

    // do the final comparison
    auto &y_actual = test_output_WS->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(y_actual[i], y_expected[i], 0.00001);

    // cleanup
    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("TestOutputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");
  }

  void testCalculationEvent() {
    const std::string outName(
        "MultipleScatteringCylinderAbsorptionEventOutput");

    // setup the test workspace
    EventWorkspace_sptr wksp =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 1,
                                                                        false);
    wksp->getAxis(0)
        ->setUnit("Wavelength"); // cheat and set the units to Wavelength
    wksp->getSpectrum(0)
        .convertTof(.09, 1.); // convert to be from 1->10 (about)
    const std::size_t NUM_EVENTS = wksp->getNumberEvents();
    AnalysisDataService::Instance().add(outName, wksp);

    // create the algorithm
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize());
    TS_ASSERT(algorithm.isInitialized());

    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("InputWorkspace", wksp));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("OutputWorkspace", outName));
    TS_ASSERT_THROWS_NOTHING(algorithm.execute());
    TS_ASSERT(algorithm.isExecuted());

    // quick checks on the output workspace
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outName));
    wksp = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
    TS_ASSERT(wksp);
    TS_ASSERT_EQUALS(wksp->getNumberEvents(), NUM_EVENTS);

    // do the final comparison - this is done by bounding
    std::vector<double> y_actual;
    wksp->getSpectrum(0).getWeights(y_actual);
    for (size_t i = 0; i < y_actual.size(); ++i) {
      TS_ASSERT_LESS_THAN(1.19811, y_actual[i]);
      TS_ASSERT_LESS_THAN(y_actual[i], 3.3324);
    }

    // cleanup
    AnalysisDataService::Instance().remove(outName);
  }

private:
  Mantid::Algorithms::MultipleScatteringCylinderAbsorption algorithm;
};

#endif /*MULTIPLE_SCATTERING_ABSORPTION_TEST_H_*/

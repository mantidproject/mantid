#ifndef APPLYTRANSMISSIONCORRECTIONTEST_H_
#define APPLYTRANSMISSIONCORRECTIONTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::UnitFactory;

class ApplyTransmissionCorrectionTest : public CxxTest::TestSuite {
public:
  void testBasics() {
    Mantid::Algorithms::ApplyTransmissionCorrection correction;

    TS_ASSERT_EQUALS(correction.name(), "ApplyTransmissionCorrection");
    TS_ASSERT_EQUALS(correction.version(), 1);
    TS_ASSERT_THROWS_NOTHING(correction.initialize());
    TS_ASSERT(correction.isInitialized());
  }

  void testExec() {
    const std::string inputWS("input_data_ws");
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_test_data.xml");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();

    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", inputWS);
    mover.setPropertyValue("ComponentName", "detector1");
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X", "0.409425");
    mover.setPropertyValue("Y", "0.002575");
    mover.execute();

    Mantid::Algorithms::ApplyTransmissionCorrection correction;
    TS_ASSERT_THROWS_NOTHING(correction.initialize());

    const std::string transWS("trans");
    Workspace2D_sptr trans_ws =
        WorkspaceCreationHelper::create2DWorkspace154(1, 1, 1);
    trans_ws->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    trans_ws->mutableY(0)[0] = 0.6;
    trans_ws->mutableE(0)[0] = 0.02;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(transWS,
                                                              trans_ws);

    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("TransmissionWorkspace", transWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", outputWS))

    TS_ASSERT_THROWS_NOTHING(correction.execute())

    TS_ASSERT(correction.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    // Spot check (multiply by counting time to be on the same scale as the IGOR
    // result)
    int id = 4 + Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(result->y(id)[0], 640.5134, 0.001)

    id = 176 + Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(result->y(id)[0], 798.8448, 0.001)

    Mantid::API::AnalysisDataService::Instance().remove(transWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testExecTransByHand() {
    const std::string inputWS("input_data_ws");

    Mantid::Algorithms::ApplyTransmissionCorrection correction;
    TS_ASSERT_THROWS_NOTHING(correction.initialize());

    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty("TransmissionValue", 0.6))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty("TransmissionError", 0.02))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", outputWS))

    TS_ASSERT_THROWS_NOTHING(correction.execute())

    TS_ASSERT(correction.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    // Spot check (multiply by counting time to be on the same scale as the IGOR
    // result)
    int id = 4 + Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(result->y(id)[0], 640.5134, 0.001)

    id = 176 + Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(result->y(id)[0], 798.8448, 0.001)

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }
};

#endif /*APPLYTRANSMISSIONCORRECTIONTEST_H_*/

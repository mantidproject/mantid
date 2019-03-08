// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef SANSSOLIDANGLETEST_H_
#define SANSSOLIDANGLETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/SANSSolidAngle.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidKernel/Unit.h"

#include "MantidDataHandling/LoadEmptyInstrument.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SANSSolidAngleTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(correction.name(), "SANSSolidAngle") }

  void testVersion() { TS_ASSERT_EQUALS(correction.version(), 1) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(correction.initialize())
    TS_ASSERT(correction.isInitialized())
  }

  void testExec() {

    const std::string wsInputName("empty_instrument_ws");
    const std::string instrument("eqsans");

    // Load empty instrument
    Mantid::DataHandling::LoadEmptyInstrument loader;
    loader.initialize();
    loader.setPropertyValue("InstrumentName", instrument);
    loader.setPropertyValue("OutputWorkspace", wsInputName);
    loader.execute();
    // Move the detector 5m away from the sample
    Mantid::DataHandling::MoveInstrumentComponent moveInstrumentComponent;
    moveInstrumentComponent.initialize();
    moveInstrumentComponent.setPropertyValue("Workspace", wsInputName);
    moveInstrumentComponent.setPropertyValue("ComponentName", "detector1");
    moveInstrumentComponent.setPropertyValue("RelativePosition", "0");
    moveInstrumentComponent.setPropertyValue("Z", "5");
    moveInstrumentComponent.execute();

    // Solid Angle correction
    const std::string wsOutputName("instrument_solid_angle_ws");
    if (!correction.isInitialized())
      correction.initialize();
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", wsInputName))
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", wsOutputName))
    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::MatrixWorkspace_sptr wsOutput;
    TS_ASSERT_THROWS_NOTHING(
        wsOutput = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                wsOutputName)))

    // Let's do some validation
    TS_ASSERT_EQUALS(wsOutput->getNumberHistograms(), 49153)
    //// SA is bigger in the center: 8.99095e-07 vs 9.4172e-07
    double correctionEdge = wsOutput->dataY(48896)[0];
    double correctionCenter = wsOutput->dataY(25984)[0];
    TS_ASSERT(correctionCenter > correctionEdge);

    Mantid::API::AnalysisDataService::Instance().remove(wsInputName);
    Mantid::API::AnalysisDataService::Instance().remove(wsOutputName);
  }

private:
  Mantid::Algorithms::SANSSolidAngle correction;
  std::string inputWS;
};

#endif /*SANSSOLIDANGLETEST_H_*/

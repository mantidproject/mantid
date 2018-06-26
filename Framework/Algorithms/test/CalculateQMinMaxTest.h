#ifndef MANTID_ALGORITHMS_CALCULATEQMINMAXTEST_H_
#define MANTID_ALGORITHMS_CALCULATEQMINMAXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateQMinMax.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"

#include <boost/cast.hpp>

using Mantid::Algorithms::CalculateQMinMax;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::API::FrameworkManager;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
using Mantid::DataHandling::MaskDetectorsInShape;
using Mantid::DataHandling::MoveInstrumentComponent;

class CalculateQMinMaxTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateQMinMaxTest *createSuite() {
    return new CalculateQMinMaxTest();
  }
  static void destroySuite(CalculateQMinMaxTest *suite) { delete suite; }

  void setUp() override { FrameworkManager::Instance(); }

  void test_init() {
    CalculateQMinMax alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    CalculateQMinMax alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setChild(true);
    MatrixWorkspace_sptr ws = create_workspace();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    ws = alg.getProperty("Workspace");
    TS_ASSERT(ws);
    const auto &run = ws->run();
    TS_ASSERT(run.hasProperty("qmin"))
    TS_ASSERT(run.hasProperty("qmax"))
    const double qmin = run.getPropertyAsSingleValue("qmin");
    const double qmax = run.getPropertyAsSingleValue("qmax");
    TS_ASSERT_DELTA(qmin, 0.03553, 1E-5)
    TS_ASSERT_DELTA(qmax, 0.88199, 1E-5)
  }

private:
  MatrixWorkspace_sptr create_workspace() {
    CreateSampleWorkspace creator;
    creator.initialize();
    creator.setChild(true);
    creator.setPropertyValue("OutputWorkspace", "__unused");
    creator.setProperty("XUnit", "Wavelength");
    creator.setProperty("NumBanks", 1);
    creator.setProperty("PixelSpacing", 0.1);
    creator.setProperty("XMin", 1.);
    creator.setProperty("XMax", 5.);
    creator.setProperty("BinWidth", 0.4);
    creator.execute();
    MatrixWorkspace_sptr sampleWS = creator.getProperty("OutputWorkspace");
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setChild(true);
    mover.setProperty("Workspace", sampleWS);
    mover.setProperty("ComponentName", "bank1");
    mover.setProperty("RelativePosition", true);
    mover.setProperty("Y", -0.5);
    mover.setProperty("X", -0.5);
    mover.execute();
    Workspace_sptr movedWS = mover.getProperty("Workspace");
    const std::string shapeXML = "<infinite-cylinder id ='A'>"
                                 "<centre x ='0' y ='0' z ='0'/>"
                                 "<axis x = '0' y = '0' z = '1'/>"
                                 "<radius val = '0.1'/>"
                                 "</infinite-cylinder>";
    MaskDetectorsInShape masker;
    masker.initialize();
    masker.setChild(true);
    masker.setProperty("Workspace", movedWS);
    masker.setPropertyValue("ShapeXML", shapeXML);
    masker.execute();
    MatrixWorkspace_sptr maskedWS = masker.getProperty("Workspace");
    return maskedWS;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEQMINMAXTEST_H_ */

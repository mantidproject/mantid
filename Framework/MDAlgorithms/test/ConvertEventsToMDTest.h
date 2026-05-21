// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidMDAlgorithms/ConvertToMD.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Geometry;

class ConvertEvents2MDEvTestHelper : public ConvertToMD {
public:
  ConvertEvents2MDEvTestHelper() {};
};

//
class ConvertEventsToMDTest : public CxxTest::TestSuite {
  std::unique_ptr<ConvertEvents2MDEvTestHelper> pAlg;

public:
  static ConvertEventsToMDTest *createSuite() { return new ConvertEventsToMDTest(); }
  static void destroySuite(ConvertEventsToMDTest *suite) { delete suite; }

  void testEventWS() {
    // set up algorithm
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", "testEvWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "testMDEvWorkspace"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", ""));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    pAlg->setPropertyValue("PreprocDetectorsWS", "");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
    pAlg->setPropertyValue("MinValues", "-10,-10,-10");
    pAlg->setPropertyValue("MaxValues", " 10, 10, 10");

    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Should finish succesfully", pAlg->isExecuted());
    Mantid::API::Workspace_sptr spws;
    TS_ASSERT_THROWS_NOTHING(spws = AnalysisDataService::Instance().retrieve("testMDEvWorkspace"));
    TSM_ASSERT(" Worskpace should be retrieved", spws.get());

    std::shared_ptr<DataObjects::MDEventWorkspace<DataObjects::MDEvent<3>, 3>> ws =
        std::dynamic_pointer_cast<DataObjects::MDEventWorkspace<DataObjects::MDEvent<3>, 3>>(spws);
    TSM_ASSERT("It should be 3D MD workspace", ws.get());

    if (ws.get()) {
      TS_ASSERT_EQUALS(900, ws->getNPoints());
    } else {
      TS_FAIL("event workspace has not beed build");
    }
    AnalysisDataService::Instance().remove("testMDEvWorkspace");
  }

  void testEventWSusingLogTimes() {
    // Add Gonio here as otherwise the NaNs in the log will cause the other test to fail
    Mantid::API::MatrixWorkspace_sptr wsEv =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testEvWS"));
    Goniometer gonio; // Add a vertical rotation axis
    gonio.pushAxis("Rot", 0., 1., 0., 0.);
    wsEv->mutableRun().setGoniometer(gonio, true);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", "testEvWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "testMDEvWorkspace"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", ""));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setProperty("UseLogTimes", true));
    pAlg->setPropertyValue("PreprocDetectorsWS", "");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues", "-10,-10,-10,-100");
    pAlg->setPropertyValue("MaxValues", " 10, 10, 10, 100");

    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Should finish succesfully", pAlg->isExecuted());
    Mantid::API::Workspace_sptr spws;
    TS_ASSERT_THROWS_NOTHING(spws = AnalysisDataService::Instance().retrieve("testMDEvWorkspace"));
    TSM_ASSERT(" Worskpace should be retrieved", spws.get());

    std::shared_ptr<DataObjects::MDEventWorkspace<DataObjects::MDEvent<4>, 4>> ws =
        std::dynamic_pointer_cast<DataObjects::MDEventWorkspace<DataObjects::MDEvent<4>, 4>>(spws);
    TSM_ASSERT("It should be 4D MD workspace", ws.get());

    if (ws.get()) {
      TS_ASSERT_EQUALS(450, ws->getNPoints()); // Expect half the events as for non-UseLogTimes
    } else {
      TS_FAIL("event workspace has not beed build");
    }
    AnalysisDataService::Instance().remove("testMDEvWorkspace");
    wsEv->mutableRun().setGoniometer(gonio, false); // Change back to not using log
  }

  ConvertEventsToMDTest() {
    FrameworkManager::Instance();

    pAlg = std::make_unique<ConvertEvents2MDEvTestHelper>();
    pAlg->initialize();

    int numHist = 10;
    int numEvents = 100;
    Mantid::API::MatrixWorkspace_sptr wsEv = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::createEventWorkspace(numHist, numEvents, numEvents, 0.0, 0.1, 5, 0));
    wsEv->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(numHist));
    wsEv->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    // any inelastic units or unit conversion using TOF needs Ei to be present
    // among properties.
    wsEv->mutableRun().addProperty("Ei", 13., "meV", true);

    // Add fake log and goniometer for useLogTimes test
    auto rotlog = std::make_unique<Kernel::TimeSeriesProperty<double>>("Rot");
    auto t0 = DateAndTime("2010-01-01T00:00:00");
    for (int i = 0; i < numEvents / 2; i++) {
      rotlog->addValue(t0 + double(i), double(i) * 0.5);
    }
    // Make half of the events have no rotation - so they should be ignored if UseLogTimes
    for (int i = numEvents / 2; i < numEvents; i++) {
      rotlog->addValue(t0 + double(i), std::numeric_limits<double>::quiet_NaN());
    }
    wsEv->mutableRun().addLogData(std::move(rotlog));
    wsEv->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(4., 4., 4., 90., 90., 90.));

    AnalysisDataService::Instance().addOrReplace("testEvWS", wsEv);
  }
};

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDAlgorithms/ConvertToMD.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

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

  ConvertEventsToMDTest() {
    FrameworkManager::Instance();

    pAlg = std::make_unique<ConvertEvents2MDEvTestHelper>();
    pAlg->initialize();

    int numHist = 10;
    Mantid::API::MatrixWorkspace_sptr wsEv = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::createRandomEventWorkspace(100, numHist, 0.1));
    wsEv->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(numHist));
    // any inelastic units or unit conversion using TOF needs Ei to be present
    // among properties.
    // wsEv->mutableRun().addProperty("Ei",13.,"meV",true);

    AnalysisDataService::Instance().addOrReplace("testEvWS", wsEv);
  }
};

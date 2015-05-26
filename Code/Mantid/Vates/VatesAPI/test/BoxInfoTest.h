#ifndef COMOPOSITE_PEAKS_BOX_INFO_TEST_H_
#define COMOPOSITE_PEAKS_BOX_INFO_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/BoxInfo.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/MDLeanEvent.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;
using namespace Mantid::Kernel;

class BoxInfoTest : public CxxTest::TestSuite {
public:
  void test_initial_recursion_depth_is_empty_for_MD_Histo() {
    // Arrange
    const std::string wsName = "MD_HISTO_WS";
    auto ws = makeFakeMDHistoWorkspace(1.0, 4, 5, 1.0, 0.1, wsName);

    // Act + Assert
    TSM_ASSERT("Should have no recursion depth for top level splitting.",
                           boost::none == Mantid::VATES::findRecursionDepthForTopLevelSplitting(wsName));

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_initial_recursion_depth_is_empty_for_MD_Event_wo_split() {
    // Arrange
    const std::string wsName = "MD_EVENT_WS";
    auto ws = makeAnyMDEW<MDLeanEvent<3>, 3>(10, 0.0, 10.0, 1, wsName);

    // Act + Assert
    TSM_ASSERT("Should have no recursion depth for top level splitting.",
               boost::none == Mantid::VATES::findRecursionDepthForTopLevelSplitting(wsName));

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_initial_recursion_depth_is_1_for_MD_Event_w_split() {
    // Arrange
    const std::string wsName = "MD_EVENT_WS_WITH_SPLITTING";
    provideMDEventWorspaceWithTopLevelSplitting(wsName);

    // Act
    auto result = Mantid::VATES::findRecursionDepthForTopLevelSplitting(wsName);
    // Assert
    TSM_ASSERT("Should have recursion depth of 1 for top level splitting.",
               result.get() == 1);

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

private:
  void provideMDEventWorspaceWithTopLevelSplitting(std::string wsName) {

    auto alg = Mantid::API::AlgorithmManager::Instance().create(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("WorkspaceType", "Event");
    alg->setPropertyValue("OutputWorkspace", wsName);
    alg->execute();

    Mantid::API::MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
    Mantid::API::Run &run = ws->mutableRun();
    auto eiLog = new PropertyWithValue<double>("Ei", 12.0);
    run.addLogData(eiLog);

    Mantid::MDAlgorithms::ConvertToMD convertAlg;
    convertAlg.setChild(true);
    convertAlg.initialize();
    convertAlg.setPropertyValue("OutputWorkspace",wsName);
    convertAlg.setProperty("InputWorkspace", ws);
    convertAlg.setProperty("QDimensions", "Q3D");
    convertAlg.setProperty("dEAnalysisMode", "Direct");
    convertAlg.setPropertyValue("MinValues","-10,-10,-10, 0");
    convertAlg.setPropertyValue("MaxValues"," 10, 10, 10, 1");
    convertAlg.setPropertyValue("TopLevelSplitting", "1");
    convertAlg.execute();

    if (!AnalysisDataService::Instance().doesExist(wsName)) {
      IMDEventWorkspace_sptr  ws_new = convertAlg.getProperty("OutputWorkspace");
      AnalysisDataService::Instance().addOrReplace(wsName, ws_new);
    }
  }
};

#endif
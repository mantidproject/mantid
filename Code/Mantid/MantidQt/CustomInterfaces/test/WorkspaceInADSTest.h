#ifndef CUSTOM_INTERFACES_WORKSPACE_IN_ADS_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_IN_ADS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/WorkspaceInADS.h"
#include "MantidDataObjects/Workspace2D.h"


using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

class WorkspaceInADSTest : public CxxTest::TestSuite
{

public:

  void testConstructorThrowsIfWorkspaceNotPresentInADS()
  {
    TSM_ASSERT_THROWS("Should have thrown. Workspace in ADS is not a matrix workspace", new WorkspaceInADS("MadeItUp"), std::runtime_error);
  }
  
  void testConstructorThrowsUnlessMatrixWorkspace()
  {
    AnalysisDataService::Instance().addOrReplace("ws", WorkspaceFactory::Instance().createPeaks());

    TSM_ASSERT_THROWS("Should have thrown. Workspace in ADS is not a matrix workspace", new WorkspaceInADS("ws") , std::invalid_argument);
  }

  void testCheckStillThereWhenThere()
  {
    Workspace_sptr ws(new Mantid::DataObjects::Workspace2D());
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    WorkspaceInADS memento("ws");
    TS_ASSERT(memento.checkStillThere());
  }

  void testCheckNotStillThereWhenNotThere()
  {
    Workspace_sptr ws(new Mantid::DataObjects::Workspace2D());
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    WorkspaceInADS memento("ws");
    AnalysisDataService::Instance().remove("ws");
    TS_ASSERT(!memento.checkStillThere());
  }

  void testFetchItSuccessful()
  {
    Workspace_sptr ws(new Mantid::DataObjects::Workspace2D());
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    WorkspaceInADS memento("ws");
    TS_ASSERT(memento.checkStillThere());
    Mantid::API::MatrixWorkspace_sptr result = memento.fetchIt();
    TS_ASSERT(result != NULL);
  }

  void testFetchItUnSuccessful()
  {
    Workspace_sptr ws(new Mantid::DataObjects::Workspace2D());
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    WorkspaceInADS memento("ws");
    AnalysisDataService::Instance().remove("ws");
    TS_ASSERT(!memento.checkStillThere());
    TS_ASSERT_THROWS(memento.fetchIt(), std::runtime_error);
  }


};

#endif
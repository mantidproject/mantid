#ifndef MDEW_EVENT_NEXUS_PRESENTER_TEST_H_ 
#define MDEW_EVENT_NEXUS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidVatesAPI/MDEWEventNexusPresenter.h"
#include "MantidAPI/FileFinder.h"

using namespace Mantid::API;
using namespace Mantid::VATES;
using namespace Mantid::MDEvents;
using namespace testing;

class MDEWEventNexusPresenterTest : public CxxTest::TestSuite
{
private:

  // Helper mock View type.
  class MockMDLoadingRebinningView : public MDLoadingRebinningView 
  {
  public:
    MOCK_CONST_METHOD0(getImplicitFunction,
      vtkImplicitFunction*());
    MOCK_CONST_METHOD0(getMaxThreshold,
      double());
    MOCK_CONST_METHOD0(getMinThreshold,
      double());
    MOCK_CONST_METHOD0(getApplyClip,
      bool());
    MOCK_CONST_METHOD0(getTimeStep,
      double());
    MOCK_CONST_METHOD0(getAppliedGeometryXML,
      const char*());
    MOCK_METHOD1(updateAlgorithmProgress,
      void(double));
    MOCK_CONST_METHOD0(getLoadInMemory, bool());
  };

  // Helper mock ActionManager type.
  class MockRebinningActionManager : public Mantid::VATES::RebinningActionManager
  {
  public:
    MOCK_METHOD1(ask, void(Mantid::VATES::RebinningIterationAction));
    MOCK_CONST_METHOD0(action, RebinningIterationAction());
    MOCK_METHOD0(reset, void());
    virtual ~MockRebinningActionManager(){}
  };

  // Helper method to return the full path to a MDEW file. Not handled by this presenter.
  static std::string getBadFile()
  {
    return FileFinder::Instance().getFullPath("MAPS_MDEW.nxs");
  }

  // Helper method to return the full path to a valid Event Nexus file.
  static std::string getGoodFile()
  {
    return FileFinder::Instance().getFullPath("CNCS_7860_event.nxs");
  }

  // Typedef for presenter hooked up to Mock view type.
  typedef MDEWEventNexusPresenter<MockMDLoadingRebinningView> Presenter;
  // Typedef for Progress Action hooked up to Mock view type.
  typedef FilterUpdateProgressAction<MockMDLoadingRebinningView> UpdateHandler;

public:

/* Should only be able to read files of EVENT-NEXUS type */
void testCannotRead()
{
  MockMDLoadingRebinningView view;

  std::string path = getBadFile();
  Presenter p(path, new MockRebinningActionManager, &view);

  TSM_ASSERT("Should NOT be able to load MDEW file", !p.canLoadFile());
}

/* Should only be able to read files of EVENT-NEXUS type */
void testCanReadFile()
{
  MockMDLoadingRebinningView view;
  
  std::string path = getGoodFile();
  Presenter p(path, new MockRebinningActionManager, &view);

  TSM_ASSERT("Should be able to load Event Nexus file", p.canLoadFile());
}

/* Test that loading algorithm works. */
void testExecuteLoad()
{
  MockMDLoadingRebinningView view;
  UpdateHandler handler(&view);

  std::string path = getGoodFile();
  Presenter p(path, new MockRebinningActionManager, &view);
  
  //Run the load algorithm via the presenter interface.
  p.executeLoad(handler);
  
  //Check that workspace is generated and inside the analysis dataservice.
  Workspace_sptr result=AnalysisDataService::Instance().retrieve("event_ws_id");
  Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);
  TS_ASSERT(eventWs->getNPoints() > 0);
  TS_ASSERT_EQUALS(3, eventWs->getNumDims());
}

};

#endif
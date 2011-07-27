#ifndef MDEW_LOADING_REBINNING_PRESENTER_TEST_H_ 
#define MDEW_LOADING_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Poco/XML/XMLException.h>

#include "MantidVatesAPI/MDEWLoadingRebinningPresenter.h"

using namespace Mantid::API;
using namespace Mantid::VATES;
using namespace Mantid::MDEvents;
using namespace testing;

class MDEWLoadingRebinningPresenterTest : public CxxTest::TestSuite
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

  /// Mock file loading algorithm
  class MockDataHandlingAlgorithm : public Mantid::API::Algorithm
  {
  public:
    MOCK_CONST_METHOD0(isInitialized, bool());
    MOCK_CONST_METHOD0(name,const std::string());
    MOCK_CONST_METHOD0(version, int());
    MOCK_METHOD0(init, void());
    MOCK_METHOD0(exec, void());
  };


  class ConcretePresenter : public MDEWLoadingRebinningPresenter<MockMDLoadingRebinningView>
  {
  public:
    ConcretePresenter(RebinningActionManager* request, MockMDLoadingRebinningView* view, std::string loadedGeometryXML) :
        MDEWLoadingRebinningPresenter<MockMDLoadingRebinningView>("somefile", request, view)
    {
      //This is what the executeLoad method will do.
      m_serializer.setGeometryXML(loadedGeometryXML);
    }
    virtual ~ConcretePresenter(){}
    bool canLoadFile() const { return true; }
    virtual void executeLoad(ProgressAction&)
    { 
      m_hasLoaded = true;
    }
  };

  // Typedef for Progress Action hooked up to Mock view type.
  typedef FilterUpdateProgressAction<MockMDLoadingRebinningView> UpdateHandler;

public:

  void testMustExecuteLoadBeforeUpdate()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    ConcretePresenter presenter(new MockRebinningActionManager, &view, "SOME_GEOM_XML");
    TSM_ASSERT_THROWS("Should throw if have NOT called ::executeLoad before ::updateModel!", presenter.updateModel(), std::runtime_error);
  }

    /*Test that when view settings have not altered from those of the model, no rebinning actions are requested upon update.*/
  void testUpdateModelWithNoChanges()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).WillOnce(Return(false));
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("SOME_GEOM_XML"));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(0); //Since nothing has changed, no requests should be made.

    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  /*Test that when view has different load_in_memory setting, request is for dataset to be reloaded and rebinned.*/
  void testUpdateModelWithDifferentInMemorySetting()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).Times(2).WillRepeatedly(Return(true)); // Changed! The model default setting will be false for this.
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("SOME_GEOM_XML"));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(ReloadAndRecalculateAll)).Times(1); //Should ask to ReloadAndRecalculate one time.

    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMaxThreshold()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).WillOnce(Return(false));
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).Times(2).WillRepeatedly(Return(1)); //Maxthreshold non-zero
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("SOME_GEOM_XML"));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Maxthreshold updated should reflect on request.
    
    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMinThreshold()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).WillOnce(Return(false));
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0)); 
    EXPECT_CALL(view, getMinThreshold()).Times(2).WillRepeatedly(Return(1)); //Minthreshold non-zero
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("SOME_GEOM_XML"));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Minthreshold updated should reflect on request.
    
    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentTimestep()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).WillOnce(Return(false));
    EXPECT_CALL(view, getTimeStep()).Times(2).WillRepeatedly(Return(1)); //Timestep updated
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("SOME_GEOM_XML"));
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Timestep updated should reflect on request.

    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }



  void testUpdateModelWithDifferntGeometryXML()
  {
    MockMDLoadingRebinningView view;
    UpdateHandler handler(&view);

    EXPECT_CALL(view, getLoadInMemory()).WillOnce(Return(false));
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return("CHANGED_GEOM_XML"));
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(_)).Times(0); //Should not get to this point. see comment below.

    ConcretePresenter presenter(pRequest, &view, "SOME_GEOM_XML");
    presenter.executeLoad(handler);

    //XML exception indicates that the geometry has been identified as different and is being parsered. Preference here to avoid 
    //writing xml strings, so xml exception sufficient to incicate that the internal MDGeometryXMLParsers are being called.
    TS_ASSERT_THROWS(presenter.updateModel(), Poco::XML::XMLException);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

};

#endif
#ifndef MDHW_LOADING_PRESENTER_TEST_H_
#define MDHW_LOADING_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"

#include "MockObjects.h"

using namespace testing;
using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDHWLoadingPresenterTest : public CxxTest::TestSuite
{

private: 

  /*
  Helper class allows the behaviour of the abstract base type to be tested. Derives from target abstract class providing 
  dummy implemenations of pure virtual methods.
  */
  class ConcreteMDHWLoadingPresenter : public MDHWLoadingPresenter
  {
  private:
    typedef MDHWLoadingPresenter BaseClass;
  public:
    ConcreteMDHWLoadingPresenter(MockMDLoadingView* view) : MDHWLoadingPresenter(view)
    {
    }

    virtual void extractMetadata(Mantid::API::IMDHistoWorkspace_sptr histoWs)
    {
      MDHWLoadingPresenter::extractMetadata(histoWs);
    }

    virtual vtkDataSet* execute(vtkDataSetFactory*, ProgressAction&)
    {
      return vtkUnstructuredGrid::New(); 
    }
    
    virtual void executeLoadMetadata()
    {
    }

    virtual bool canReadFile() const
    {
      return true;
    }

    virtual bool shouldLoad()
    {
      //Forwarding method
      return BaseClass::shouldLoad();
    }

    ~ConcreteMDHWLoadingPresenter(){}
  };


public:

void testShouldLoadFirstTimeRound()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(0);
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDHWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Should NOT request load on second usage. Should have it's state syncrhonised with view and the view hasn't changed!", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testTimeChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(0);
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2)
    .WillOnce(Return(0)) 
    .WillOnce(Return(1));// Time has changed on 2nd call
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDHWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Time has changed, but that shouldn't trigger load", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testLoadInMemoryChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(0);
  EXPECT_CALL(view, getLoadInMemory()).Times(2)
    .WillOnce(Return(true)) 
    .WillOnce(Return(false)); // Load in memory changed
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDHWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Load in memory changed. this SHOULD trigger re-load", presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testhasTDimensionWhenIntegrated()
{
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;

  ConcreteMDHWLoadingPresenter presenter(view);

  //Test that it does work when setup.
  Mantid::API::Workspace_sptr ws = get3DWorkspace(true, false); //Integrated T Dimension
  presenter.extractMetadata(boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));

  TSM_ASSERT("This is a 4D workspace with an integrated T dimension", !presenter.hasTDimensionAvailable());
}

void testHasTDimensionWhenNotIntegrated()
{
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;

  ConcreteMDHWLoadingPresenter presenter(view);

  //Test that it does work when setup. 
  Mantid::API::Workspace_sptr ws = get3DWorkspace(false, false); //Non-integrated T Dimension
  presenter.extractMetadata(boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));

  TSM_ASSERT("This is a 4D workspace with an integrated T dimension", presenter.hasTDimensionAvailable());
}

};

#endif

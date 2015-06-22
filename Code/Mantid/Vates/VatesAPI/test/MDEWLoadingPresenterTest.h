#ifndef MDEW_LOADING_PRESENTER_TEST_H_
#define MDEW_LOADING_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MockObjects.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::API;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWLoadingPresenterTest : public CxxTest::TestSuite
{

private: 

  /*
  Helper class allows the behaviour of the abstract base type to be tested. Derives from target abstract class providing 
  dummy implemenations of pure virtual methods.
  */
  class ConcreteMDEWLoadingPresenter : public MDEWLoadingPresenter
  {
  private:
    typedef MDEWLoadingPresenter BaseClass;

  public:

    virtual void extractMetadata(Mantid::API::IMDEventWorkspace_sptr eventWs)
    {
      return MDEWLoadingPresenter::extractMetadata(eventWs);
    }
  
    ConcreteMDEWLoadingPresenter(MockMDLoadingView* view) : MDEWLoadingPresenter(view)
    {
    }

    virtual vtkDataSet* execute(vtkDataSetFactory*, ProgressAction&, ProgressAction&)
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

    virtual bool canLoadFileBasedOnExtension(const std::string& filename, const std::string& expectedExtension) const
    {
      //Forwarding method.
      return BaseClass::canLoadFileBasedOnExtension(filename, expectedExtension);
    }

    ~ConcreteMDEWLoadingPresenter(){}
  };


public:

void testShouldLoadFirstTimeRound()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Should NOT request load on second usage. Should have it's state syncrhonised with view and the view hasn't changed!", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testTimeChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2)
    .WillOnce(Return(0)) 
    .WillOnce(Return(1));// Time has changed on 2nd call
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Time has changed, but that shouldn't trigger load", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testLoadInMemoryChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2)
    .WillOnce(Return(true)) 
    .WillOnce(Return(false)); // Load in memory changed
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Load in memory changed. this SHOULD trigger re-load", presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testDepthChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2)
    .WillOnce(Return(10)) 
    .WillOnce(Return(100)); // Recursion depth changed.
  EXPECT_CALL(view, getLoadInMemory()).Times(2);
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_,_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Depth has changed, but that shouldn't trigger load", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

  void testhasTDimensionWhenIntegrated()
  {
    //Setup view
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);
    
    //Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(true, true); //Integrated T Dimension
    presenter.extractMetadata(boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension", !presenter.hasTDimensionAvailable());
  }

  void testHasTDimensionWhenNotIntegrated()
  {
    //Setup view
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);
    
    //Test that it does work when setup. 
    Mantid::API::Workspace_sptr ws = get3DWorkspace(false, true); //Non-integrated T Dimension
    presenter.extractMetadata(boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension", presenter.hasTDimensionAvailable());
  }

  void testHasTimeLabelWithTDimension()
  {
    //Setup view
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);

    //Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(false, true); //Non-integrated T Dimension
    presenter.extractMetadata(boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT_EQUALS("This is a 4D workspace with a T dimension", "D (A)", presenter.getTimeStepLabel());
  }

  void testCanSetAxisLabelsFrom3DData()
  {
    //Setup view
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);

    //Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(true, true);
    presenter.extractMetadata(boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));
    vtkDataSet *ds = vtkUnstructuredGrid::New();
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A (A)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B (A)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C (A)");
  }

  void testCanSetAxisLabelsFrom4DData()
  {
    //Setup view
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);

    //Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(false, true);
    presenter.extractMetadata(boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));
    vtkDataSet *ds = vtkUnstructuredGrid::New();
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A (A)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B (A)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C (A)");
  }

  void testCanLoadFileBasedOnExtension()
  {
    MockMDLoadingView* view = new MockMDLoadingView;

    ConcreteMDEWLoadingPresenter presenter(view);

    // constructive tests
    TSM_ASSERT("Should be an exact match", presenter.canLoadFileBasedOnExtension("somefile.nxs", ".nxs"));
    TSM_ASSERT("Should lowercase uppercase extension", presenter.canLoadFileBasedOnExtension("somefile.NXS", ".nxs"));
    TSM_ASSERT("Should strip off whitespace", presenter.canLoadFileBasedOnExtension("somefile.nxs ", ".nxs"));
    // destructive tests
    TSM_ASSERT("Extensions do not match, should return false.", !presenter.canLoadFileBasedOnExtension("somefile.nx", ".nxs"));

    delete view;
  }


};

#endif

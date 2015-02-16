#ifndef MDHW_NEXUS_LOADING_PRESENTER_TEST_H_
#define MDHW_NEXUS_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidAPI/FileFinder.h"
#include "MantidVatesAPI/MDHWNexusLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

using namespace Mantid::VATES;
using namespace testing;
//=====================================================================================
// Functional tests
//=====================================================================================
class MDHWNexusLoadingPresenterTest : public CxxTest::TestSuite
{

private:

  // Helper method to return the full path to a real nexus file that is the correct format for this functionality.
  static std::string getSuitableFile()
  {
    std::string temp = Mantid::API::FileFinder::Instance().getFullPath("SEQ_MDHW.nxs");
    return temp;
  }
  
  // Helper method to return the full path to a real nexus file that is the wrong format for this functionality.
  static std::string getUnhandledFile()
  {
    std::string temp = Mantid::API::FileFinder::Instance().getFullPath("CNCS_7860_event.nxs");
    return temp;
  }

public:

void testConstructWithEmptyFileThrows()
{
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", MDHWNexusLoadingPresenter(new MockMDLoadingView, ""), std::invalid_argument);
}

void testConstructWithNullViewThrows()
{
  MockMDLoadingView*  pView = NULL;

  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", MDHWNexusLoadingPresenter(pView, "some_file"), std::invalid_argument);
}

void testConstruct()
{
  TSM_ASSERT_THROWS_NOTHING("Object should be created without exception.", MDHWNexusLoadingPresenter(new MockMDLoadingView, getSuitableFile()));
}

void testCanReadFile()
{
  MDHWNexusLoadingPresenter presenter(new MockMDLoadingView, getUnhandledFile());
  TSM_ASSERT("A file of this type cannot and should not be read by this presenter!.", !presenter.canReadFile());
}

void testExecution()
{
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(0));
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(testing::Return(true)); 
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));

  //Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;
  
  //Create the presenter and runit!
  MDHWNexusLoadingPresenter presenter(view, getSuitableFile());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);

  TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
  TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid", std::string(product->GetClassName()));
  TSM_ASSERT("No field data!", NULL != product->GetFieldData());
  TSM_ASSERT_EQUALS("Two arrays expected on field data, one for XML and one for JSON!", 2, product->GetFieldData()->GetNumberOfArrays());
  TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
  TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
  TS_ASSERT(!presenter.getWorkspaceTypeName().empty());

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  product->Delete();
}

void testCallHasTDimThrows()
{
  MDHWNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable(), std::runtime_error);
}

void testCallGetTDimensionValuesThrows()
{
  MDHWNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getTimeStepValues(), std::runtime_error);
}

void testCallGetGeometryThrows()
{
  MDHWNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getGeometryXML(), std::runtime_error);
}

void testGetWorkspaceTypeName()
{
  MDHWNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_EQUALS("Characterisation Test Failed", "", presenter.getWorkspaceTypeName());
}

void testTimeLabel()
{
  // Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(0));
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  // Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));

  // Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;

  //Create the presenter and runit!
  MDHWNexusLoadingPresenter presenter(view, getSuitableFile());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);
  TSM_ASSERT_EQUALS("Time label should be exact.",
                    presenter.getTimeStepLabel(), "DeltaE (DeltaE)");

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  product->Delete();
}

void testAxisLabels()
{
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(0));
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));

  //Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;

  //Create the presenter and runit!
  MDHWNexusLoadingPresenter presenter(view, getSuitableFile());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);
  TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));
  TSM_ASSERT_EQUALS("X Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForX"),
                    "[H,0,0] (in 1.992 A^-1)");
  TSM_ASSERT_EQUALS("Y Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForY"),
                    "[0,K,0] (in 1.992 A^-1)");
  TSM_ASSERT_EQUALS("Z Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForZ"),
                    "[0,0,L] (in 1.087 A^-1)");

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  product->Delete();
}

};
#endif

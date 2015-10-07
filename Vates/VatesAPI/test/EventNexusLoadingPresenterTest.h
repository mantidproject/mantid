#ifndef EVENT_NEXUS_LOADING_PRESENTER_TEST_H_
#define EVENT_NEXUS_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidAPI/FileFinder.h"
#include "MantidVatesAPI/EventNexusLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

using namespace Mantid::VATES;
using namespace testing;
//=====================================================================================
// Functional tests
//=====================================================================================
class EventNexusLoadingPresenterTest : public CxxTest::TestSuite
{

private:

  // Helper method to return the full path to a real nexus file that is the correct format for this functionality.
  static std::string getSuitableFile()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("CNCS_7860_event.nxs");
  }
  
  // Helper method to return the full path to a real nexus file that is the wrong format for this functionality.
  static std::string getUnhandledFile()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("emu00006473.nxs");
  }

public:

void testConstructWithEmptyFileThrows()
{
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", EventNexusLoadingPresenter(new MockMDLoadingView, ""), std::invalid_argument);
}

void testConstructWithNullViewThrows()
{
  MockMDLoadingView*  pView = NULL;

  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", EventNexusLoadingPresenter(pView, "some_file"), std::invalid_argument);
}

void testConstruct()
{
  TSM_ASSERT_THROWS_NOTHING("Object should be created without exception.", EventNexusLoadingPresenter(new MockMDLoadingView, getSuitableFile()));
}

void testCanReadFile()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getUnhandledFile());
  TSM_ASSERT("A file of this type cannot and should not be read by this presenter!.", !presenter.canReadFile());
}

void testExecution()
{
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1)); 
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)); 
  EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  NiceMock<MockProgressAction> mockLoadingProgressUpdate;
  MockProgressAction mockDrawingProgressUpdate;

  //Create the presenter and runit!
  EventNexusLoadingPresenter presenter(view, getSuitableFile());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressUpdate, mockDrawingProgressUpdate);

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

void testGetTDimension()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT("EventNexus MDEW are created in fixed 3D.", !presenter.hasTDimensionAvailable());
}

void testCallGetTDimensionValuesThrows()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getTimeStepValues(), std::runtime_error);
}

void testCallGetGeometryThrows()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getGeometryXML(), std::runtime_error);
}

void testExecuteLoadMetadata()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  presenter.executeLoadMetadata();
  TSM_ASSERT_THROWS("Should always throw. Algorithm fixed to create 3 dimensions.", presenter.getTimeStepValues(), std::runtime_error);
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable());
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.getGeometryXML());
}

void testGetWorkspaceTypeName()
{
  EventNexusLoadingPresenter presenter(new MockMDLoadingView, getSuitableFile());
  TSM_ASSERT_EQUALS("Characterisation Test Failed", "", presenter.getWorkspaceTypeName());
}



};
#endif

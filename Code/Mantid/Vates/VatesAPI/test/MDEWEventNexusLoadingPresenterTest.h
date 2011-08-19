#ifndef MDEW_EVENT_NEXUS_LOADING_PRESENTER_TEST_H_
#define MDEW_EVENT_NEXUS_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidAPI/FileFinder.h"
#include "MantidVatesAPI/MDEWEventNexusLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

using namespace Mantid::VATES;
using namespace testing;
//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWEventNexusLoadingPresenterTest : public CxxTest::TestSuite
{

private:

  // Helper method to return the full path to a real nexus file that is the correct format for this functionality.
  static std::string getSuitableFile()
  {
    std::string temp = Mantid::API::FileFinder::Instance().getFullPath("MAPS_MDEW.nxs");
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
  MockMDLoadingView view;

  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", MDEWEventNexusLoadingPresenter<MockMDLoadingView>(&view, ""), std::invalid_argument);
}

void testConstructWithNullViewThrows()
{
  MockMDLoadingView*  pView = NULL;

  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", MDEWEventNexusLoadingPresenter<MockMDLoadingView>(pView, "some_file"), std::invalid_argument);
}

void testConstruct()
{
  MockMDLoadingView view;

  TSM_ASSERT_THROWS_NOTHING("Object should be created without exception.", MDEWEventNexusLoadingPresenter<MockMDLoadingView>(&view, getSuitableFile()));
}


void testCanReadFile()
{
  MockMDLoadingView view;
  MDEWEventNexusLoadingPresenter<MockMDLoadingView> presenter(&view, getUnhandledFile());
  TSM_ASSERT("A file of this type cannot and should not be read by this presenter!.", !presenter.canReadFile());
}

void testExecution()
{
  //Setup view
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(AtLeast(1)); 
  EXPECT_CALL(view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(testing::Return(true)); 
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create()).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  //Setup progress updates object
  FilterUpdateProgressAction<MockMDLoadingView> progressAction(&view);

  //Create the presenter and runit!
  MDEWEventNexusLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, progressAction);

  TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
  TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid", std::string(product->GetClassName()));
  TSM_ASSERT("No field data!", NULL != product->GetFieldData());
  TSM_ASSERT_EQUALS("One array expected on field data!", 1, product->GetFieldData()->GetNumberOfArrays());
  TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
  TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());

  TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  product->Delete();
}

void testCallHasTDimThrows()
{
  MockMDLoadingView view;
  MDEWEventNexusLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable(), std::runtime_error);
}

void testCallGetTDimensionValuesThrows()
{
  MockMDLoadingView view;
  MDEWEventNexusLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getTimeStepValues(), std::runtime_error);
}

void testCallGetGeometryThrows()
{
  MockMDLoadingView view;
  MDEWEventNexusLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getGeometryXML(), std::runtime_error);
}

};
#endif
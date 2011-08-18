#ifndef SQW_LOADING_PRESENTER_TEST_H_
#define SQW_LOADING_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidAPI/FileFinder.h"
#include "MantidVatesAPI/SQWLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class SQWLoadingPresenterTest : public CxxTest::TestSuite
{

private:

  // Helper method to return the full path to a real sqw file.
  static std::string getSuitableFile()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("test_horace_reader.sqw");
  }
  
  // Helper method to return the full path to a file that is invalid.
  static std::string getUnhandledFile()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("emu00006473.nxs");
  }

public:

void testConstructWithEmptyFileThrows()
{
  MockMDLoadingView view;
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", SQWLoadingPresenter<MockMDLoadingView>(&view, ""), std::invalid_argument);
}

void testConstructWithNullViewThrows()
{
  MockMDLoadingView*  pView = NULL;
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", SQWLoadingPresenter<MockMDLoadingView>(pView, "some_file"), std::invalid_argument);
}

void testConstruct()
{
  MockMDLoadingView view;
  TSM_ASSERT_THROWS_NOTHING("Object should be created without exception.", SQWLoadingPresenter<MockMDLoadingView>(&view, getSuitableFile()));
}

void testCanReadFile()
{
  MockMDLoadingView view;

  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT("Should be readable, valid SQW file.", presenter.canReadFile());
}

void testCanReadFileWithDifferentCaseExtension()
{
  MockMDLoadingView view;

  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, "other.Sqw");
  TSM_ASSERT("Should be readable, only different in case.", presenter.canReadFile());
}

void testCannotReadFileWithWrongExtension()
{
  MockMDLoadingView view;

  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getUnhandledFile());
  TSM_ASSERT("Should NOT be readable, completely wrong file type.", !presenter.canReadFile());
}

void testExecution()
{
  using namespace testing;
   //Setup view
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(AtLeast(1)); 
  EXPECT_CALL(view, getLoadInMemory()).Times(AtLeast(1)); 
  EXPECT_CALL(view, getTime()).Times(AtLeast(1));
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create()).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  //Setup progress updates object
  FilterUpdateProgressAction<MockMDLoadingView> progressAction(&view);

  //Create the presenter and runit!
  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
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
  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable(), std::runtime_error);
}

void testCallGetTDimensionValuesThrows()
{
  MockMDLoadingView view;
  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getTimeStepValues(), std::runtime_error);
}

void testCallGetGeometryThrows()
{
  MockMDLoadingView view;
  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getGeometryXML(), std::runtime_error);
}

void testExecuteLoadMetadata()
{
  MockMDLoadingView view;
  SQWLoadingPresenter<MockMDLoadingView> presenter(&view, getSuitableFile());
  presenter.executeLoadMetadata();
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.getTimeStepValues());
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable());
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.getGeometryXML());
}

};

#endif
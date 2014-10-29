#ifndef SQW_LOADING_PRESENTER_TEST_H_
#define SQW_LOADING_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"
#include <fstream>

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
  static std::string getSuitableFileNamePath()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("test_horace_reader.sqw");
  }
  
  // Helper method to return the full path to a file that is invalid.
  static std::string getUnhandledFileNamePath()
  {
    return Mantid::API::FileFinder::Instance().getFullPath("emu00006473.nxs");
  }

  // Helper method. Create the expected backend filename + path using the same rules used internally in SQWLoadingPresenter.
  static std::string getFileBackend(std::string fileName)
  {
    size_t pos = fileName.find(".");
    return fileName.substr(0, pos) + ".nxs";
  }

public:

void setUp()
{
    std::remove(getFileBackend(getSuitableFileNamePath()).c_str()); // Clean out any pre-existing backend files.
}

void testConstructWithEmptyFileThrows()
{
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", SQWLoadingPresenter(new MockMDLoadingView, ""), std::invalid_argument);
}

void testConstructWithNullViewThrows()
{
  MockMDLoadingView*  pView = NULL;
  TSM_ASSERT_THROWS("Should throw if an empty file string is given.", SQWLoadingPresenter(pView, "some_file"), std::invalid_argument);
}

void testConstruct()
{
  TSM_ASSERT_THROWS_NOTHING("Object should be created without exception.", SQWLoadingPresenter(new MockMDLoadingView, getSuitableFileNamePath()));
}

void testCanReadFile()
{
  MockMDLoadingView view;

  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  TSM_ASSERT("Should be readable, valid SQW file.", presenter.canReadFile());
}

void testCanReadFileWithDifferentCaseExtension()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, "other.Sqw");
  TSM_ASSERT("Should be readable, only different in case.", presenter.canReadFile());
}

void testCannotReadFileWithWrongExtension()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getUnhandledFileNamePath());
  TSM_ASSERT("Should NOT be readable, completely wrong file type.", !presenter.canReadFile());
}

void testExecutionInMemory()
{
  using namespace testing;
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1)); 
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(Return(true)); // View setup to request loading in memory.
  EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  //Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;
  //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
  EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

  //Create the presenter and runit!
  SQWLoadingPresenter presenter(view, getSuitableFileNamePath());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);

  std::string fileNameIfGenerated = getFileBackend(getSuitableFileNamePath());
  std::ifstream fileExists(fileNameIfGenerated.c_str(), ifstream::in);
  TSM_ASSERT("File Backend SHOULD NOT be generated.",  !fileExists.good());

  TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
  TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid", std::string(product->GetClassName()));
  TSM_ASSERT("No field data!", NULL != product->GetFieldData());
  TSM_ASSERT_EQUALS("One array expected on field data!", 1, product->GetFieldData()->GetNumberOfArrays());
  TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
  TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
  TS_ASSERT(!presenter.getWorkspaceTypeName().empty());

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  TSM_ASSERT("Bad usage of loading algorithm progress updates", Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));

  product->Delete();
}

void testCallHasTDimThrows()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable(), std::runtime_error);
}

void testCallGetTDimensionValuesThrows()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getTimeStepValues(), std::runtime_error);
}

void testCallGetGeometryThrows()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  TSM_ASSERT_THROWS("Should throw. Execute not yet run.", presenter.getGeometryXML(), std::runtime_error);
}

void testExecuteLoadMetadata()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  presenter.executeLoadMetadata();
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.getTimeStepValues());
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.hasTDimensionAvailable());
  TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.", presenter.getGeometryXML());
}

void testGetWorkspaceTypeName()
{
  SQWLoadingPresenter presenter(new MockMDLoadingView, getSuitableFileNamePath());
  TSM_ASSERT_EQUALS("Characterisation Test Failed", "", presenter.getWorkspaceTypeName());
}

void testTimeLabel()
{
  using namespace testing;
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1));
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(Return(true)); // View setup to request loading in memory.
  EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  //Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;
  //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
  EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

  //Create the presenter and runit!
  SQWLoadingPresenter presenter(view, getSuitableFileNamePath());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);
  TSM_ASSERT_EQUALS("Time label should be exact.",
                    presenter.getTimeStepLabel(), "en (meV)");

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  TSM_ASSERT("Bad usage of loading algorithm progress updates", Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));

  product->Delete();
}

void testAxisLabels()
{
  using namespace testing;
  //Setup view
  MockMDLoadingView* view = new MockMDLoadingView;
  EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1));
  EXPECT_CALL(*view, getLoadInMemory()).Times(AtLeast(1)).WillRepeatedly(Return(true)); // View setup to request loading in memory.
  EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
  EXPECT_CALL(*view, updateAlgorithmProgress(_,_)).Times(AnyNumber());

  //Setup rendering factory
  MockvtkDataSetFactory factory;
  EXPECT_CALL(factory, initialize(_)).Times(1);
  EXPECT_CALL(factory, create(_)).WillOnce(testing::Return(vtkUnstructuredGrid::New()));
  EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

  //Setup progress updates objects
  MockProgressAction mockLoadingProgressAction;
  MockProgressAction mockDrawingProgressAction;
  //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
  EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

  //Create the presenter and runit!
  SQWLoadingPresenter presenter(view, getSuitableFileNamePath());
  presenter.executeLoadMetadata();
  vtkDataSet* product = presenter.execute(&factory, mockLoadingProgressAction, mockDrawingProgressAction);
  TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));
  TSM_ASSERT_EQUALS("X Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForX"),
                    "qx (A^-1)");
  TSM_ASSERT_EQUALS("Y Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForY"),
                    "qy (A^-1)");
  TSM_ASSERT_EQUALS("Z Label should match exactly",
                    getStringFieldDataValue(product, "AxisTitleForZ"),
                    "qz (A^-1)");

  TS_ASSERT(Mock::VerifyAndClearExpectations(view));
  TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  TSM_ASSERT("Bad usage of loading algorithm progress updates", Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));

  product->Delete();
}

};

#endif

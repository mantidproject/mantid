// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SQW_LOADING_PRESENTER_TEST_H_
#define SQW_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include "MockObjects.h"
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FileFinder.h"

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/SQWLoadingPresenter.h"

using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class SQWLoadingPresenterTest : public CxxTest::TestSuite {

private:
  // Helper method to return the full path to a real sqw file.
  static std::string getSuitableFileNamePath() {
    return Mantid::API::FileFinder::Instance().getFullPath(
        "test_horace_reader.sqw");
  }

  // Helper method to return the full path to a file that is invalid.
  static std::string getUnhandledFileNamePath() {
    return Mantid::API::FileFinder::Instance().getFullPath("emu00006473.nxs");
  }

  // Helper method. Create the expected backend filename + path using the same
  // rules used internally in SQWLoadingPresenter.
  static std::string getFileBackend(std::string fileName) {
    size_t pos = fileName.find(".");
    return fileName.substr(0, pos) + ".nxs";
  }

public:
  void setUp() override {
    std::remove(getFileBackend(getSuitableFileNamePath())
                    .c_str()); // Clean out any pre-existing backend files.
  }

  void testConstructWithEmptyFileThrows() {
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    TSM_ASSERT_THROWS("Should throw if an empty file string is given.",
                      SQWLoadingPresenter(std::move(view), ""),
                      const std::invalid_argument &);
  }

  void testConstructWithNullViewThrows() {
    TSM_ASSERT_THROWS("Should throw if an empty file string is given.",
                      SQWLoadingPresenter(nullptr, "some_file"),
                      const std::invalid_argument &);
  }

  void testConstruct() {
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    TSM_ASSERT_THROWS_NOTHING(
        "Object should be created without exception.",
        SQWLoadingPresenter(std::move(view), getSuitableFileNamePath()));
  }

  void testCanReadFile() {
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    SQWLoadingPresenter presenter(std::move(view), getSuitableFileNamePath());
    TSM_ASSERT("Should be readable, valid SQW file.", presenter.canReadFile());
  }

  void testCanReadFileWithDifferentCaseExtension() {
    auto view = std::make_unique<MockMDLoadingView>();
    SQWLoadingPresenter presenter(std::move(view), "other.Sqw");
    TSM_ASSERT("Should be readable, only different in case.",
               presenter.canReadFile());
  }

  void testCannotReadFileWithWrongExtension() {
    auto view = std::make_unique<MockMDLoadingView>();
    SQWLoadingPresenter presenter(std::move(view), getUnhandledFileNamePath());
    TSM_ASSERT("Should NOT be readable, completely wrong file type.",
               !presenter.canReadFile());
  }

  void testExecutionInMemory() {
    using namespace testing;
    // Setup view
    auto view = std::make_unique<MockMDLoadingView>();
    EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1));
    EXPECT_CALL(*view, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(
            Return(true)); // View setup to request loading in memory.
    EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));
    EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    // Create the presenter and runit!
    SQWLoadingPresenter presenter(std::move(view), getSuitableFileNamePath());
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);

    std::string fileNameIfGenerated = getFileBackend(getSuitableFileNamePath());
    std::ifstream fileExists(fileNameIfGenerated.c_str(), ifstream::in);
    TSM_ASSERT("File Backend SHOULD NOT be generated.", !fileExists.good());

    TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
    TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid",
                      std::string(product->GetClassName()));
    TSM_ASSERT("No field data!", NULL != product->GetFieldData());
    TSM_ASSERT_EQUALS(
        "Two arrays expected on field data, one for XML and one for JSON!", 2,
        product->GetFieldData()->GetNumberOfArrays());
    TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
    TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
    TS_ASSERT(!presenter.getWorkspaceTypeName().empty());

    TS_ASSERT(Mock::VerifyAndClearExpectations(view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

    TSM_ASSERT("Bad usage of loading algorithm progress updates",
               Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));
  }

  void testCallHasTDimThrows() {
    SQWLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFileNamePath());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.hasTDimensionAvailable(),
                      const std::runtime_error &);
  }

  void testCallGetTDimensionValuesThrows() {
    SQWLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFileNamePath());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getTimeStepValues(),
                      const std::runtime_error &);
  }

  void testCallGetGeometryThrows() {
    SQWLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFileNamePath());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getGeometryXML(), const std::runtime_error &);
  }

  void testExecuteLoadMetadata() {
    SQWLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFileNamePath());
    presenter.executeLoadMetadata();
    TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.",
                              presenter.getTimeStepValues());
    TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.",
                              presenter.hasTDimensionAvailable());
    TSM_ASSERT_THROWS_NOTHING("Should throw. Execute not yet run.",
                              presenter.getGeometryXML());
  }

  void testGetWorkspaceTypeName() {
    SQWLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFileNamePath());
    TSM_ASSERT_EQUALS("Characterisation Test Failed", "",
                      presenter.getWorkspaceTypeName());
  }

  void testTimeLabel() {
    using namespace testing;
    // Setup view
    auto view = std::make_unique<MockMDLoadingView>();
    EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1));
    EXPECT_CALL(*view, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(
            Return(true)); // View setup to request loading in memory.
    EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));
    EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    // Create the presenter and runit!
    SQWLoadingPresenter presenter(std::move(view), getSuitableFileNamePath());
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);
    TSM_ASSERT_EQUALS("Time label should be exact.",
                      presenter.getTimeStepLabel(), "en (meV)");

    TS_ASSERT(Mock::VerifyAndClearExpectations(view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

    TSM_ASSERT("Bad usage of loading algorithm progress updates",
               Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));
  }

  void testAxisLabels() {
    using namespace testing;
    // Setup view
    auto view = std::make_unique<MockMDLoadingView>();
    EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(1));
    EXPECT_CALL(*view, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(
            Return(true)); // View setup to request loading in memory.
    EXPECT_CALL(*view, getTime()).Times(AtLeast(1));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));
    EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockLoadingProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    // Create the presenter and runit!
    SQWLoadingPresenter presenter(std::move(view), getSuitableFileNamePath());
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForX"),
                      "Q_sample_x ($\\AA^{-1}$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForY"),
                      "Q_sample_y ($\\AA^{-1}$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForZ"),
                      "Q_sample_z ($\\AA^{-1}$)");

    TS_ASSERT(Mock::VerifyAndClearExpectations(view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
    TSM_ASSERT("Bad usage of loading algorithm progress updates",
               Mock::VerifyAndClearExpectations(&mockLoadingProgressAction));
  }
};

#endif

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDHW_NEXUS_LOADING_PRESENTER_TEST_H_
#define MDHW_NEXUS_LOADING_PRESENTER_TEST_H_

#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include <cxxtest/TestSuite.h>
#include <vtkDataArray.h>
#include <vtkMatrix4x4.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FileFinder.h"

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDHWNexusLoadingPresenter.h"

#include <limits>

using namespace Mantid::VATES;
using namespace testing;
//=====================================================================================
// Functional tests
//=====================================================================================
class MDHWNexusLoadingPresenterTest : public CxxTest::TestSuite {

private:
  // Helper method to return the full path to a real nexus file that is the
  // correct format for this functionality.
  static std::string getSuitableFile() {
    std::string temp =
        Mantid::API::FileFinder::Instance().getFullPath("SEQ_MDHW.nxs");
    return temp;
  }

  // Helper method to return the full path to a real nexus file that is the
  // wrong format for this functionality.
  static std::string getUnhandledFile() {
    std::string temp =
        Mantid::API::FileFinder::Instance().getFullPath("CNCS_7860_event.nxs");
    return temp;
  }

  vtkSmartPointer<vtkDataSet> doExecute(std::string filename,
                                        bool performAsserts = true) {
    // Setup view
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getTime()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(AtLeast(0));
    EXPECT_CALL(*mockView, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;

    // Create the presenter and runit!
    MDHWNexusLoadingPresenter presenter(std::move(view), filename);
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);

    TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
    if (performAsserts) {
      TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid",
                        std::string(product->GetClassName()));
      TSM_ASSERT("No field data!", NULL != product->GetFieldData());
      TSM_ASSERT_EQUALS(
          "Two arrays expected on field data, one for XML and one for JSON!", 2,
          product->GetFieldData()->GetNumberOfArrays());
      TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
      TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
      TS_ASSERT(!presenter.getWorkspaceTypeName().empty());

      TS_ASSERT(Mock::VerifyAndClearExpectations(mockView));
      TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
    }
    return product;
  }

public:
  void testConstructWithEmptyFileThrows() {
    TSM_ASSERT_THROWS(
        "Should throw if an empty file string is given.",
        MDHWNexusLoadingPresenter(std::make_unique<MockMDLoadingView>(), ""),
        const std::invalid_argument &);
  }

  void testConstructWithNullViewThrows() {
    TSM_ASSERT_THROWS("Should throw if an empty file string is given.",
                      MDHWNexusLoadingPresenter(nullptr, "some_file"),
                      const std::invalid_argument &);
  }

  void testConstruct() {
    TSM_ASSERT_THROWS_NOTHING(
        "Object should be created without exception.",
        MDHWNexusLoadingPresenter(std::make_unique<MockMDLoadingView>(),
                                  getSuitableFile()));
  }

  void testCanReadFile() {
    MDHWNexusLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                        getUnhandledFile());
    TSM_ASSERT(
        "A file of this type cannot and should not be read by this presenter!.",
        !presenter.canReadFile());
  }

  void testExecution() {
    auto filename = getSuitableFile();
    vtkSmartPointer<vtkDataSet> product;
    product = doExecute(filename);
  }

  void testExecutionWithLegacyFile() {
    // This is not a good unit test but almost an integration test to
    // check if the COB Matrix is affected when loading legacy files.
    // Move this to system tests if it becomes availbale for C++ code.

    auto filename = Mantid::API::FileFinder::Instance().getFullPath(
        "test_non_orthogonal.nxs");

    // Setup progress updates objects
    NiceMock<MockProgressAction> mockLoadingProgressAction;
    NiceMock<MockProgressAction> mockDrawingProgressAction;

    // Setup view
    auto view = std::make_unique<MockMDLoadingView>();
    EXPECT_CALL(*view, getTime()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getRecursionDepth()).Times(AtLeast(0));
    EXPECT_CALL(*view, getLoadInMemory())
        .Times(AtLeast(0))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Create the presenter as in the vtkMDHWSource
    auto normalizationOption = Mantid::VATES::VisualNormalization::AutoSelect;
    MDHWNexusLoadingPresenter presenter(std::move(view), filename);
    const double time = 0.0;
    auto factory = boost::make_shared<vtkMDHistoHex4DFactory<TimeToTimeStep>>(
        normalizationOption, time);

    factory
        ->setSuccessor(
            std::make_unique<vtkMDHistoHexFactory>(normalizationOption))
        .setSuccessor(
            std::make_unique<vtkMDHistoQuadFactory>(normalizationOption))
        .setSuccessor(
            std::make_unique<vtkMDHistoLineFactory>(normalizationOption))
        .setSuccessor(std::make_unique<vtkMD0DFactory>());

    presenter.executeLoadMetadata();
    auto product = presenter.execute(factory.get(), mockLoadingProgressAction,
                                     mockDrawingProgressAction);

    // Set the COB
    try {
      auto workspaceProvider =
          std::make_unique<ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
      presenter.makeNonOrthogonal(product, std::move(workspaceProvider),
                                  &mockDrawingProgressAction);
    } catch (...) {
      // Add the standard change of basis matrix and set the boundaries
      presenter.setDefaultCOBandBoundaries(product);
    }

    // Assert that the COB matrix is a skewed matrix with the values below.
    double expectedElements[16] = {1.0,
                                   0.50029480938126836,
                                   -0.0001890681732465397,
                                   0.0,
                                   0.0,
                                   0.86585512859032043,
                                   0.0015546654605598377,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.99999877363351386,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   1.0};
    vtkSmartPointer<vtkMatrix4x4> cobMatrix =
        vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(product);

    TS_ASSERT_DELTA(cobMatrix->GetElement(0, 0), expectedElements[0],
                    std::numeric_limits<double>::epsilon());

    TS_ASSERT_DELTA(cobMatrix->GetElement(0, 1), expectedElements[1],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(0, 2), expectedElements[2],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(0, 3), expectedElements[3],
                    std::numeric_limits<double>::epsilon());

    TS_ASSERT_DELTA(cobMatrix->GetElement(1, 0), expectedElements[4],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(1, 1), expectedElements[5],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(1, 2), expectedElements[6],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(1, 3), expectedElements[7],
                    std::numeric_limits<double>::epsilon());

    TS_ASSERT_DELTA(cobMatrix->GetElement(2, 0), expectedElements[8],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(2, 1), expectedElements[9],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(2, 2), expectedElements[10],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(2, 3), expectedElements[11],
                    std::numeric_limits<double>::epsilon());

    TS_ASSERT_DELTA(cobMatrix->GetElement(3, 0), expectedElements[12],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(3, 1), expectedElements[13],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(3, 2), expectedElements[14],
                    std::numeric_limits<double>::epsilon());
    TS_ASSERT_DELTA(cobMatrix->GetElement(3, 3), expectedElements[15],
                    std::numeric_limits<double>::epsilon());
  }

  void testCallHasTDimThrows() {
    MDHWNexusLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                        getSuitableFile());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.hasTDimensionAvailable(),
                      const std::runtime_error &);
  }

  void testCallGetTDimensionValuesThrows() {
    MDHWNexusLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                        getSuitableFile());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getTimeStepValues(),
                      const std::runtime_error &);
  }

  void testCallGetGeometryThrows() {
    MDHWNexusLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                        getSuitableFile());
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getGeometryXML(), const std::runtime_error &);
  }

  void testGetWorkspaceTypeName() {
    MDHWNexusLoadingPresenter presenter(std::make_unique<MockMDLoadingView>(),
                                        getSuitableFile());
    TSM_ASSERT_EQUALS("Characterisation Test Failed", "",
                      presenter.getWorkspaceTypeName());
  }

  void testTimeLabel() {
    // Setup view
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getTime()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(AtLeast(0));
    EXPECT_CALL(*mockView, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;

    // Create the presenter and runit!
    MDHWNexusLoadingPresenter presenter(std::move(view), getSuitableFile());
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);
    TSM_ASSERT_EQUALS("Time label should be exact.",
                      presenter.getTimeStepLabel(), "DeltaE (DeltaE)");

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testAxisLabels() {
    // Setup view
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getTime()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(AtLeast(0));
    EXPECT_CALL(*mockView, getLoadInMemory())
        .Times(AtLeast(1))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(testing::Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;

    // Create the presenter and runit!
    MDHWNexusLoadingPresenter presenter(std::move(view), getSuitableFile());
    presenter.executeLoadMetadata();
    auto product = presenter.execute(&factory, mockLoadingProgressAction,
                                     mockDrawingProgressAction);
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));

    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForX"),
                      "[H,0,0] ($in$ $1.992$ $\\AA^{-1}$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForY"),
                      "[0,K,0] ($in$ $1.992$ $\\AA^{-1}$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForZ"),
                      "[0,0,L] ($in$ $1.087$ $\\AA^{-1}$)");

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }
};
#endif

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDEW_IN_MEMORY_LOADING_PRESENTER_TEST_H
#define MDEW_IN_MEMORY_LOADING_PRESENTER_TEST_H

#include "MantidAPI/FileFinder.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/WarningSuppressions.h"

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace testing;

class MDEWInMemoryLoadingPresenterTest : public CxxTest::TestSuite {

private:
  // Helper type. Mocks a Workspace Provider.
  class MockWorkspaceProvider : public Mantid::VATES::WorkspaceProvider {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(canProvideWorkspace, bool(std::string));
    MOCK_CONST_METHOD1(fetchWorkspace,
                       Mantid::API::Workspace_sptr(std::string));
    MOCK_CONST_METHOD1(disposeWorkspace, void(std::string));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };

  // Helper method. Generates and returns a valid IMDEventWorkspace
  static Mantid::API::Workspace_sptr getReal4DWorkspace() {
    AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
    alg->initialize();
    alg->setRethrows(true);
    alg->setPropertyValue(
        "Filename",
        Mantid::API::FileFinder::Instance().getFullPath("MAPS_MDEW.nxs"));
    alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
    alg->setProperty("FileBackEnd", false);
    alg->execute();
    return AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
  }

  // Helper method. Generates a non-IMDEventWorkspace.
  static Mantid::API::Workspace_sptr getBadWorkspace() {
    // Return a table workspace.
    return WorkspaceFactory::Instance().createTable();
  }

public:
  void testConstructWithNullViewThrows() {
    TSM_ASSERT_THROWS(
        "Should throw with null view.",
        MDEWInMemoryLoadingPresenter(nullptr, new MockWorkspaceProvider, "_"),
        const std::invalid_argument &);
  }

  void testConstructWithNullRepositoryThrows() {
    TSM_ASSERT_THROWS("Should throw with null repository.",
                      MDEWInMemoryLoadingPresenter(
                          std::make_unique<MockMDLoadingView>(), nullptr, "_"),
                      const std::invalid_argument &);
  }

  void testConstructWithEmptyWsNameThrows() {
    std::string emptyName = "";
    TSM_ASSERT_THROWS(
        "Should throw with empty Workspace name.",
        MDEWInMemoryLoadingPresenter(std::make_unique<MockMDLoadingView>(),
                                     new MockWorkspaceProvider, emptyName),
        const std::invalid_argument &);
  }

  void testConstruction() {
    TS_ASSERT_THROWS_NOTHING(MDEWInMemoryLoadingPresenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_"));
  }

  void testCanLoadWithInvalidName() {
    auto repository = std::make_unique<MockWorkspaceProvider>();
    EXPECT_CALL(*repository, canProvideWorkspace(_))
        .WillOnce(Return(
            false)); // No matter what the argument, always returns false.

    // Give a dummy name corresponding to the workspace.
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), repository.release(), "_");

    TSM_ASSERT("Should indicate that the workspace cannot be read-out since "
               "the name is not in the Repository.",
               !presenter.canReadFile());
  }

  void testCanLoadWithWrongWsType() {
    auto repository = std::make_unique<MockWorkspaceProvider>();
    Mantid::API::Workspace_sptr badWs =
        getBadWorkspace(); // Not an IMDEventWorkspace.
    EXPECT_CALL(*repository, canProvideWorkspace(_))
        .WillOnce(
            Return(true)); // No matter what the argument, always returns true.
    EXPECT_CALL(*repository, fetchWorkspace(_)).WillOnce(Return(badWs));

    // Give a dummy name corresponding to the workspace.
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), repository.release(), "_");

    TSM_ASSERT("Should indicate that the workspace cannot be read-out since it "
               "is not of the right type.",
               !presenter.canReadFile());
  }

  void testCanLoadSucceeds() {
    auto repository = std::make_unique<MockWorkspaceProvider>();
    Mantid::API::Workspace_sptr goodWs = getReal4DWorkspace();
    EXPECT_CALL(*repository, canProvideWorkspace(_))
        .WillOnce(
            Return(true)); // No matter what the argument, always returns true.
    EXPECT_CALL(*repository, fetchWorkspace(_)).WillOnce(Return(goodWs));

    // Give a dummy name corresponding to the workspace.
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), repository.release(), "_");

    TSM_ASSERT("Should have worked! Workspace is of correct type and "
               "repository says ws is present.!",
               presenter.canReadFile());
  }

  void testExtractMetadata() {
    auto repository = std::make_unique<MockWorkspaceProvider>();
    Mantid::API::Workspace_sptr ws = getReal4DWorkspace();
    EXPECT_CALL(*repository, fetchWorkspace(_))
        .Times(1)
        .WillRepeatedly(Return(ws));

    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), repository.release(), "_");

    // Test that it doesn't work when not setup.
    TSM_ASSERT_THROWS("::executeLoadMetadata is critical to setup, should "
                      "throw if not run first.",
                      presenter.getGeometryXML(), const std::runtime_error &);

    // Test that it does work when setup.
    presenter.executeLoadMetadata();
    TSM_ASSERT("Should export geometry xml metadata on request.",
               !presenter.getGeometryXML().empty());
    TSM_ASSERT("Should export instrument metadata on request",
               presenter.getInstrument().empty())
  }

  void testExecution() {
    // Setup view
    std::unique_ptr<MDLoadingView> view = std::make_unique<MockMDLoadingView>();
    auto mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(1);
    EXPECT_CALL(*mockView, getLoadInMemory())
        .Times(0); // Not a question that needs asking for this presenter type.
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    // Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_))
        .WillOnce(Return(vtkSmartPointer<vtkUnstructuredGrid>::New()));
    EXPECT_CALL(factory, setRecursionDepth(_)).Times(1);

    auto repository = std::make_unique<MockWorkspaceProvider>();
    Mantid::API::Workspace_sptr ws = getReal4DWorkspace();
    EXPECT_CALL(*repository, fetchWorkspace(_))
        .Times(2)
        .WillRepeatedly(Return(ws));

    // Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;

    // Create the presenter and run it!
    MDEWInMemoryLoadingPresenter presenter(std::move(view),
                                           repository.release(), "_");
    presenter.executeLoadMetadata();
    vtkSmartPointer<vtkDataSet> product = presenter.execute(
        &factory, mockLoadingProgressAction, mockDrawingProgressAction);

    TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
    TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid",
                      std::string(product->GetClassName()));
    TSM_ASSERT("No field data!", NULL != product->GetFieldData());
    TSM_ASSERT_EQUALS(
        "One array expected on field data, one for XML and one for JSON!", 2,
        product->GetFieldData()->GetNumberOfArrays());
    TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
    TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
    TS_ASSERT(!presenter.getWorkspaceTypeName().empty());
    TSM_ASSERT("Special coordinate metadata failed.",
               -1 < presenter.getSpecialCoordinates());
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testCallHasTDimThrows() {
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.hasTDimensionAvailable(),
                      const std::runtime_error &);
  }

  void testCallGetTDimensionValuesThrows() {
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getTimeStepValues(),
                      const std::runtime_error &);
  }

  void testCallGetGeometryThrows() {
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getGeometryXML(), const std::runtime_error &);
  }

  void testGetWorkspaceTypeName() {
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_");
    TSM_ASSERT_EQUALS("Characterisation Test Failed", "",
                      presenter.getWorkspaceTypeName());
  }

  void testGetSpecialCoordinates() {
    MDEWInMemoryLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>(), new MockWorkspaceProvider, "_");
    TSM_ASSERT_EQUALS("Characterisation Test Failed", -1,
                      presenter.getSpecialCoordinates());
  }
};

#endif

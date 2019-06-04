// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDEW_LOADING_PRESENTER_TEST_H_
#define MDEW_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"

#include "MockObjects.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::API;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWLoadingPresenterTest : public CxxTest::TestSuite {

private:
  /*
  Helper class allows the behaviour of the abstract base type to be tested.
  Derives from target abstract class providing
  dummy implemenations of pure virtual methods.
  */
  class ConcreteMDEWLoadingPresenter : public MDEWLoadingPresenter {
  private:
    using BaseClass = MDEWLoadingPresenter;

  public:
    void
    extractMetadata(const Mantid::API::IMDEventWorkspace &eventWs) override {
      return MDEWLoadingPresenter::extractMetadata(eventWs);
    }

    ConcreteMDEWLoadingPresenter(std::unique_ptr<MDLoadingView> view)
        : MDEWLoadingPresenter(std::move(view)) {}

    vtkSmartPointer<vtkDataSet> execute(vtkDataSetFactory *, ProgressAction &,
                                        ProgressAction &) override {
      return vtkSmartPointer<vtkUnstructuredGrid>::New();
    }

    void executeLoadMetadata() override {}

    bool canReadFile() const override { return true; }

    bool shouldLoad() override {
      // Forwarding method
      return BaseClass::shouldLoad();
    }

    bool canLoadFileBasedOnExtension(
        const std::string &filename,
        const std::string &expectedExtension) const override {
      // Forwarding method.
      return BaseClass::canLoadFileBasedOnExtension(filename,
                                                    expectedExtension);
    }

    ~ConcreteMDEWLoadingPresenter() override {}
  };

public:
  void testShouldLoadFirstTimeRound() {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(2);
    EXPECT_CALL(*mockView, getLoadInMemory()).Times(2);
    EXPECT_CALL(*mockView, getTime()).Times(2);
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(0);

    ConcreteMDEWLoadingPresenter presenter(std::move(view));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Should NOT request load on second usage. Should have it's "
               "state syncrhonised with view and the view hasn't changed!",
               !presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(mockView));
  }

  void testTimeChanged() {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(2);
    EXPECT_CALL(*mockView, getLoadInMemory()).Times(2);
    EXPECT_CALL(*mockView, getTime())
        .Times(2)
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // Time has changed on 2nd call
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(0);

    ConcreteMDEWLoadingPresenter presenter(std::move(view));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Time has changed, but that shouldn't trigger load",
               !presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(mockView));
  }

  void testLoadInMemoryChanged() {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getRecursionDepth()).Times(2);
    EXPECT_CALL(*mockView, getLoadInMemory())
        .Times(2)
        .WillOnce(Return(true))
        .WillOnce(Return(false)); // Load in memory changed
    EXPECT_CALL(*mockView, getTime()).Times(2);
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(0);

    ConcreteMDEWLoadingPresenter presenter(std::move(view));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Load in memory changed. this SHOULD trigger re-load",
               presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(mockView));
  }

  void testDepthChanged() {
    std::unique_ptr<MDLoadingView> view =
        std::make_unique<MockMDLoadingView>();
    MockMDLoadingView *mockView = dynamic_cast<MockMDLoadingView *>(view.get());
    EXPECT_CALL(*mockView, getRecursionDepth())
        .Times(2)
        .WillOnce(Return(10))
        .WillOnce(Return(100)); // Recursion depth changed.
    EXPECT_CALL(*mockView, getLoadInMemory()).Times(2);
    EXPECT_CALL(*mockView, getTime()).Times(2);
    EXPECT_CALL(*mockView, updateAlgorithmProgress(_, _)).Times(0);

    ConcreteMDEWLoadingPresenter presenter(std::move(view));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Depth has changed, but that shouldn't trigger load",
               !presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(mockView));
  }

  void testhasTDimensionWhenIntegrated() {
    // Setup view
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(true, true); // Integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension",
               !presenter.hasTDimensionAvailable());
  }

  void testHasTDimensionWhenNotIntegrated() {
    // Setup view
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(false, true); // Non-integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension",
               presenter.hasTDimensionAvailable());
  }

  void testHasTimeLabelWithTDimension() {
    // Setup view
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(false, true); // Non-integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

    TSM_ASSERT_EQUALS("This is a 4D workspace with a T dimension", "D (A)",
                      presenter.getTimeStepLabel());
  }

  void testCanSetAxisLabelsFrom3DData() {
    // Setup view
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(true, true);
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));
    auto ds = vtkSmartPointer<vtkDataSet>::Take(vtkUnstructuredGrid::New());
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A ($A$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B ($A$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C ($A$)");
  }

  void testCanSetAxisLabelsFrom4DData() {
    // Setup view
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(false, true);
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));
    auto ds = vtkSmartPointer<vtkDataSet>::Take(vtkUnstructuredGrid::New());
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A ($A$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B ($A$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C ($A$)");
  }

  void testCanLoadFileBasedOnExtension() {
    ConcreteMDEWLoadingPresenter presenter(
        std::make_unique<MockMDLoadingView>());

    // constructive tests
    TSM_ASSERT("Should be an exact match",
               presenter.canLoadFileBasedOnExtension("somefile.nxs", ".nxs"));
    TSM_ASSERT("Should lowercase uppercase extension",
               presenter.canLoadFileBasedOnExtension("somefile.NXS", ".nxs"));
    TSM_ASSERT("Should strip off whitespace",
               presenter.canLoadFileBasedOnExtension("somefile.nxs ", ".nxs"));
    // destructive tests
    TSM_ASSERT("Extensions do not match, should return false.",
               !presenter.canLoadFileBasedOnExtension("somefile.nx", ".nxs"));
  }
};

#endif

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDHW_LOADING_PRESENTER_TEST_H_
#define MDHW_LOADING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidDataObjects/MDHistoWorkspace.h"

#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"

#include "MockObjects.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::API;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDHWLoadingPresenterTest : public CxxTest::TestSuite {

private:
  /*
  Helper class allows the behaviour of the abstract base type to be tested.
  Derives from target abstract class providing
  dummy implemenations of pure virtual methods.
  */
  class ConcreteMDHWLoadingPresenter : public MDHWLoadingPresenter {
  private:
    using BaseClass = MDHWLoadingPresenter;

  public:
    ConcreteMDHWLoadingPresenter(std::unique_ptr<MDLoadingView> view)
        : MDHWLoadingPresenter(std::move(view)) {}

    void
    extractMetadata(const Mantid::API::IMDHistoWorkspace &histoWs) override {
      MDHWLoadingPresenter::extractMetadata(histoWs);
    }

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

    ~ConcreteMDHWLoadingPresenter() override {}
  };

public:
  void testShouldLoadFirstTimeRound() {
    auto view = new MockMDLoadingView();
    EXPECT_CALL(*view, getRecursionDepth()).Times(0);
    EXPECT_CALL(*view, getLoadInMemory()).Times(2);
    EXPECT_CALL(*view, getTime()).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(0);

    std::unique_ptr<MDLoadingView> uniqueView(
        dynamic_cast<MDLoadingView *>(view));
    ConcreteMDHWLoadingPresenter presenter(std::move(uniqueView));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Should NOT request load on second usage. Should have it's "
               "state syncrhonised with view and the view hasn't changed!",
               !presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(view));
  }

  void testTimeChanged() {
    auto view = new MockMDLoadingView();
    EXPECT_CALL(*view, getRecursionDepth()).Times(0);
    EXPECT_CALL(*view, getLoadInMemory()).Times(2);
    EXPECT_CALL(*view, getTime())
        .Times(2)
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // Time has changed on 2nd call
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(0);

    std::unique_ptr<MDLoadingView> uniqueView(
        dynamic_cast<MDLoadingView *>(view));
    ConcreteMDHWLoadingPresenter presenter(std::move(uniqueView));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Time has changed, but that shouldn't trigger load",
               !presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(view));
  }

  void testLoadInMemoryChanged() {
    auto view = new MockMDLoadingView();
    EXPECT_CALL(*view, getRecursionDepth()).Times(0);
    EXPECT_CALL(*view, getLoadInMemory())
        .Times(2)
        .WillOnce(Return(true))
        .WillOnce(Return(false)); // Load in memory changed
    EXPECT_CALL(*view, getTime()).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, updateAlgorithmProgress(_, _)).Times(0);

    std::unique_ptr<MDLoadingView> uniqueView(
        dynamic_cast<MDLoadingView *>(view));
    ConcreteMDHWLoadingPresenter presenter(std::move(uniqueView));
    TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
    TSM_ASSERT("Load in memory changed. this SHOULD trigger re-load",
               presenter.shouldLoad());

    TSM_ASSERT("View not used as expected.",
               Mock::VerifyAndClearExpectations(view));
  }

  void testhasTDimensionWhenIntegrated() {
    ConcreteMDHWLoadingPresenter presenter(
        std::make_unique<NiceMock<MockMDLoadingView>>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(true, false); // Integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension",
               !presenter.hasTDimensionAvailable());
  }

  void testHasTDimensionWhenNotIntegrated() {
    ConcreteMDHWLoadingPresenter presenter(
        std::make_unique<NiceMock<MockMDLoadingView>>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(false, false); // Non-integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));

    TSM_ASSERT("This is a 4D workspace with an integrated T dimension",
               presenter.hasTDimensionAvailable());
  }

  void testHasTimeLabelWithTDimension() {
    ConcreteMDHWLoadingPresenter presenter(
        std::make_unique<NiceMock<MockMDLoadingView>>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws =
        get3DWorkspace(false, false); // Non-integrated T Dimension
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));

    TSM_ASSERT_EQUALS("This is a 4D workspace with a T dimension", "D (A)",
                      presenter.getTimeStepLabel());
  }

  void testCanSetAxisLabelsFrom3DData() {
    ConcreteMDHWLoadingPresenter presenter(
        std::make_unique<NiceMock<MockMDLoadingView>>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(true, false);
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));
    vtkDataSet *ds = vtkUnstructuredGrid::New();
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A ($A$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B ($A$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C ($A$)");
  }

  void testCanSetAxisLabelsFrom4DData() {
    ConcreteMDHWLoadingPresenter presenter(
        std::make_unique<NiceMock<MockMDLoadingView>>());

    // Test that it does work when setup.
    Mantid::API::Workspace_sptr ws = get3DWorkspace(false, false);
    presenter.extractMetadata(
        *boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws));
    vtkDataSet *ds = vtkUnstructuredGrid::New();
    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(ds));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForX"), "A ($A$)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForY"), "B ($A$)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(ds, "AxisTitleForZ"), "C ($A$)");
  }

  Mantid::API::IMDHistoWorkspace_sptr
  makeHistoWorkspace(const std::vector<int> &shape) {

    IAlgorithm *create =
        FrameworkManager::Instance().createAlgorithm("CreateMDHistoWorkspace");
    create->setChild(true);
    create->initialize();

    const std::string allNames[5] = {"A", "B", "C", "D", "E"};
    const std::string allUnits[5] = {"AU", "BU", "CU", "DU", "EU"};

    std::vector<std::string> names;
    std::vector<std::string> units;
    size_t flatSize = 1;
    std::vector<double> extents;
    for (size_t i = 0; i < shape.size(); ++i) {
      flatSize *= shape[i];
      names.push_back(allNames[i]);
      units.push_back(allUnits[i]);
      extents.push_back(-10);
      extents.push_back(10);
    }

    create->setProperty("SignalInput", std::vector<double>(flatSize, 1));
    create->setProperty("ErrorInput", std::vector<double>(flatSize, 1));

    create->setProperty("Dimensionality", int(shape.size()));
    create->setProperty("Extents", extents);
    create->setProperty("NumberOfBins", shape);
    create->setProperty("Names", names);
    create->setProperty("Units", units);
    create->setPropertyValue("OutputWorkspace", "dummy");
    create->execute();
    IMDHistoWorkspace_sptr outWs = create->getProperty("OutputWorkspace");
    return outWs;
  }

  void test_transpose_not_needed() {

    // return outWs;
    int shape[4] = {10, 10,
                    1}; // Well behaved input workspace. Integrated dim at end.
    std::vector<int> shapeVec(shape, shape + 3);
    auto inWs = makeHistoWorkspace(shapeVec);

    IMDHistoWorkspace_sptr targetWs;
    MDHWLoadingPresenter::transposeWs(inWs, targetWs);

    TS_ASSERT_EQUALS(targetWs->getNumDims(), inWs->getNumDims());
    TS_ASSERT_EQUALS(targetWs->getNPoints(), inWs->getNPoints())
    TS_ASSERT_EQUALS(targetWs->getDimension(0)->getName(),
                     inWs->getDimension(0)->getName());
    TS_ASSERT_EQUALS(targetWs->getDimension(1)->getName(),
                     inWs->getDimension(1)->getName());
    TS_ASSERT_EQUALS(targetWs->getDimension(2)->getName(),
                     inWs->getDimension(2)->getName());
  }

  void test_transpose_rules_applied() {

    // return outWs;
    int shape[4] = {10, 10, 1,
                    10}; // Inproper input workspace. Needs transpose!
    std::vector<int> shapeVec(shape, shape + 4);
    auto inWs = makeHistoWorkspace(shapeVec);

    IMDHistoWorkspace_sptr targetWs;
    MDHWLoadingPresenter::transposeWs(inWs, targetWs);

    TS_ASSERT_EQUALS(targetWs->getNumDims(), inWs->getNumDims());
    TS_ASSERT_EQUALS(targetWs->getNPoints(), inWs->getNPoints())
    TS_ASSERT_EQUALS(targetWs->getDimension(0)->getName(),
                     inWs->getDimension(0)->getName());
    TS_ASSERT_EQUALS(targetWs->getDimension(1)->getName(),
                     inWs->getDimension(1)->getName());
    TSM_ASSERT_EQUALS("Integrated dims should be shifted to end",
                      targetWs->getDimension(2)->getName(),
                      inWs->getDimension(3)->getName());
    TSM_ASSERT_EQUALS("Integrated dims on the end",
                      targetWs->getDimension(3)->getName(),
                      inWs->getDimension(2)->getName());
  }
};

#endif

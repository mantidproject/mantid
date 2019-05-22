// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetModelMock.h"
#include "EnggDiffMultiRunFittingWidgetViewMock.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;

using namespace MantidQt::CustomInterfaces;
using testing::Return;

namespace {
API::MatrixWorkspace_sptr createSampleWorkspace() {
  return API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
}

void addBankID(API::MatrixWorkspace_sptr ws, const size_t bankID) {
  auto addLogAlg =
      API::FrameworkManager::Instance().createAlgorithm("AddSampleLog");
  addLogAlg->initialize();
  addLogAlg->setProperty("Workspace", ws);
  addLogAlg->setPropertyValue("LogName", "bankid");
  addLogAlg->setPropertyValue("LogText", std::to_string(bankID));
  addLogAlg->setPropertyValue("LogType", "Number");
  addLogAlg->execute();
}
} // namespace

class EnggDiffMultiRunFittingWidgetPresenterTest : public CxxTest::TestSuite {
public:
  void test_addFittedPeaks() {
    auto presenter = setUpPresenter();
    const auto ws = createSampleWorkspace();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockModel, addFittedPeaks(runLabel, ws)).Times(1);

    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(ws));
    EXPECT_CALL(*m_mockView, reportPlotInvalidFocusedRun(testing::_)).Times(0);
    EXPECT_CALL(*m_mockView, resetCanvas()).Times(1);
    EXPECT_CALL(*m_mockView, plotFocusedRun(testing::_)).Times(1);
    EXPECT_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockView, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_mockModel, getFittedPeaks(testing::_)).Times(0);

    presenter->addFittedPeaks(runLabel, ws);
    assertMocksUsedCorrectly();
  }

  void test_addFocusedRun() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws = createSampleWorkspace();
    addBankID(ws, 2);
    const RunLabel runLabel("0", 2);

    const std::vector<RunLabel> workspaceLabels({runLabel});
    EXPECT_CALL(*m_mockModel, getAllWorkspaceLabels())
        .Times(1)
        .WillOnce(Return(workspaceLabels));

    EXPECT_CALL(*m_mockView, updateRunList(workspaceLabels));
    presenter->addFocusedRun(ws);
    assertMocksUsedCorrectly();
  }

  void test_loadRunUpdatesView() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws = createSampleWorkspace();
    addBankID(ws, 2);

    const RunLabel runLabel("0", 2);
    const std::vector<RunLabel> workspaceLabels({runLabel});
    ON_CALL(*m_mockModel, getAllWorkspaceLabels())
        .WillByDefault(Return(workspaceLabels));
    EXPECT_CALL(*m_mockView, updateRunList(workspaceLabels));

    presenter->addFocusedRun(ws);
    assertMocksUsedCorrectly();
  }

  void test_getFittedPeaks() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockModel, getFittedPeaks(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));

    presenter->getFittedPeaks(runLabel);
    assertMocksUsedCorrectly();
  }

  void test_getFocusedRun() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));

    presenter->getFocusedRun(runLabel);
    assertMocksUsedCorrectly();
  }

  void test_selectValidRunWithoutFittedPeaks() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(createSampleWorkspace()));

    EXPECT_CALL(*m_mockView, reportPlotInvalidFocusedRun(testing::_)).Times(0);
    EXPECT_CALL(*m_mockView, resetCanvas()).Times(1);
    EXPECT_CALL(*m_mockView, plotFocusedRun(testing::_)).Times(1);

    ON_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .WillByDefault(Return(false));
    EXPECT_CALL(*m_mockView, plotFittedPeaks(testing::_)).Times(0);

    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunInvalid() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockView, reportPlotInvalidFocusedRun(runLabel)).Times(1);
    EXPECT_CALL(*m_mockView, resetCanvas()).Times(0);

    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectValidRunWithFittedPeaks() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    ON_CALL(*m_mockView, getSelectedRunLabel()).WillByDefault(Return(runLabel));

    const auto sampleWorkspace = createSampleWorkspace();
    ON_CALL(*m_mockModel, getFocusedRun(runLabel))
        .WillByDefault(Return(sampleWorkspace));

    ON_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .WillByDefault(Return(true));
    ON_CALL(*m_mockView, showFitResultsSelected()).WillByDefault(Return(true));
    EXPECT_CALL(*m_mockModel, getFittedPeaks(runLabel))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));
    EXPECT_CALL(*m_mockView, reportPlotInvalidFittedPeaks(testing::_)).Times(0);
    EXPECT_CALL(*m_mockView, plotFittedPeaks(testing::_)).Times(1);

    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunDoesNothingWhenNoRunSelected() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(boost::none));
    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_plotPeaksStateChangedUpdatesPlot() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*m_mockView, resetCanvas()).Times(1);
    EXPECT_CALL(*m_mockView, plotFocusedRun(testing::_)).Times(1);

    EXPECT_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(false));

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification::
                          PlotPeaksStateChanged);
    assertMocksUsedCorrectly();
  }

  void test_plotPeaksStateChangedDoesNotCrashWhenNoRunSelected() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockModel, getFocusedRun(testing::_)).Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification::
                          PlotPeaksStateChanged);
    assertMocksUsedCorrectly();
  }

  void test_removeRun() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel("123", 1);
    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*m_mockModel, removeRun(runLabel));

    const std::vector<RunLabel> runLabels({runLabel});
    EXPECT_CALL(*m_mockModel, getAllWorkspaceLabels())
        .Times(1)
        .WillOnce(Return(runLabels));
    EXPECT_CALL(*m_mockView, updateRunList(runLabels));
    EXPECT_CALL(*m_mockView, resetCanvas());

    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::RemoveRun);
    assertMocksUsedCorrectly();
  }

  void test_removeRunDoesNothingWhenNoRunSelected() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockModel, removeRun(testing::_)).Times(0);
    presenter->notify(
        IEnggDiffMultiRunFittingWidgetPresenter::Notification::RemoveRun);
    assertMocksUsedCorrectly();
  }

  void test_plotToSeparateWindowDoesNothingWhenNoRunSelected() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockView, reportNoRunSelectedForPlot()).Times(1);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification::
                          PlotToSeparateWindow);
    assertMocksUsedCorrectly();
  }

  void test_plotToSeparateWindowValidFocusedRunNoFittedPeaks() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel("123", 1);

    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));

    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*m_mockView, reportNoRunSelectedForPlot()).Times(0);
    EXPECT_CALL(*m_mockView, reportPlotInvalidFocusedRun(testing::_)).Times(0);

    EXPECT_CALL(*m_mockView, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(false));

    EXPECT_CALL(*m_mockView, plotToSeparateWindow(
                                 "123_1_external_plot",
                                 boost::make_optional(false, std::string())))
        .Times(1);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification::
                          PlotToSeparateWindow);
    assertMocksUsedCorrectly();
  }

  void test_plotToSeparateWindowWithFittedPeaks() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel("123", 1);

    EXPECT_CALL(*m_mockView, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));

    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*m_mockView, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockModel, hasFittedPeaksForRun(runLabel))
        .Times(1)
        .WillOnce(Return(true));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleFittedPeaks(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockModel, getFittedPeaks(runLabel))
        .Times(1)
        .WillOnce(Return(sampleFittedPeaks));
    EXPECT_CALL(*m_mockView, reportPlotInvalidFittedPeaks(testing::_)).Times(0);

    EXPECT_CALL(*m_mockView,
                plotToSeparateWindow("123_1_external_plot",
                                     boost::optional<std::string>(
                                         "123_1_fitted_peaks_external_plot")))
        .Times(1);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification::
                          PlotToSeparateWindow);
    assertMocksUsedCorrectly();
  }

  void test_getAllRunLabelsDelegatesToView() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockView, getAllRunLabels());
    presenter->getAllRunLabels();
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffMultiRunFittingWidgetModel *m_mockModel;
  MockEnggDiffMultiRunFittingWidgetView *m_mockView;

  std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> setUpPresenter() {
    auto mockModel_uptr = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetModel>>();
    m_mockModel = mockModel_uptr.get();

    m_mockView = new testing::NiceMock<MockEnggDiffMultiRunFittingWidgetView>();

    return Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetPresenter>(
        std::move(mockModel_uptr), m_mockView);
  }

  void assertMocksUsedCorrectly() {
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModel));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockView));
    if (m_mockView) {
      delete m_mockView;
    }
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

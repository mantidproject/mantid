#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetModelMock.h"
#include "EnggDiffMultiRunFittingWidgetViewMock.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;

using namespace MantidQt::CustomInterfaces;
using testing::Return;

class EnggDiffMultiRunFittingWidgetPresenterTest : public CxxTest::TestSuite {
public:
  void test_addFittedPeaks() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockModelPtr, addFittedPeaks(123, 1, ws)).Times(1);

    presenter->addFittedPeaks(123, 1, ws);
    assertMocksUsedCorrectly();
  }

  void test_addFocusedRun() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockModelPtr, addFocusedRun(123, 1, ws)).Times(1);

    const std::vector<std::pair<int, size_t>> workspaceLabels(
        {std::make_pair(123, 1)});
    EXPECT_CALL(*m_mockModelPtr, getAllWorkspaceLabels())
        .Times(1)
        .WillOnce(Return(workspaceLabels));
    EXPECT_CALL(*m_mockViewPtr, updateRunList(workspaceLabels));

    presenter->addFocusedRun(123, 1, ws);
    assertMocksUsedCorrectly();
  }

  void test_getFittedPeaks() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModelPtr, getFittedPeaks(123, 1)).Times(1);

    presenter->getFittedPeaks(123, 1);
    assertMocksUsedCorrectly();
  }

  void test_getFocusedRun() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1)).Times(1);

    presenter->getFocusedRun(123, 1);
    assertMocksUsedCorrectly();
  }

  void test_selectRunValidNoFittedPeaks() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(std::make_pair(123, 1)));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockViewPtr, resetCanvas()).Times(1);
    EXPECT_CALL(*m_mockViewPtr, plotFocusedRun(testing::_)).Times(1);

    EXPECT_CALL(*m_mockModelPtr, hasFittedPeaksForRun(123, 1))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_mockViewPtr, plotFittedPeaks(testing::_)).Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunInvalid() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(std::make_pair(123, 1)));
    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1))
        .Times(1)
        .WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockViewPtr,
                userError("Invalid focused run identifier",
                          "Tried to access invalid run, run number 123 and "
                          "bank ID 1. Please contact the development team with "
                          "this message")).Times(1);
    EXPECT_CALL(*m_mockViewPtr, resetCanvas()).Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunValidWithFittedPeaks() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(std::make_pair(123, 1)));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockViewPtr, resetCanvas()).Times(1);
    EXPECT_CALL(*m_mockViewPtr, plotFocusedRun(testing::_)).Times(1);

    EXPECT_CALL(*m_mockModelPtr, hasFittedPeaksForRun(123, 1))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockViewPtr, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockModelPtr, getFittedPeaks(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));
    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockViewPtr, plotFittedPeaks(testing::_)).Times(1);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffMultiRunFittingWidgetModel *m_mockModelPtr;
  MockEnggDiffMultiRunFittingWidgetView *m_mockViewPtr;

  std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> setUpPresenter() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetModel>>();
    m_mockModelPtr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetView>>();
    m_mockViewPtr = mockView.get();

    std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> pres_uptr(
        new EnggDiffMultiRunFittingWidgetPresenter(std::move(mockModel),
                                                   std::move(mockView)));
    return pres_uptr;
  }

  void assertMocksUsedCorrectly() {
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModelPtr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockViewPtr));
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

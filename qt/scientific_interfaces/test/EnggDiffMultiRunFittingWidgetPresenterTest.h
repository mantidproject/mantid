#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetModelMock.h"
#include "EnggDiffMultiRunFittingWidgetViewMock.h"

#include "MantidAPI/WorkspaceFactory.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;

using namespace MantidQt::CustomInterfaces;
using testing::Return;

class EnggDiffMultiRunFittingWidgetPresenterTest : public CxxTest::TestSuite {
public:
  void test_addFocusedWorkspace() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockViewPtr, getFocusedWorkspaceToAdd())
        .Times(1)
        .WillOnce(Return(ws));
    EXPECT_CALL(*m_mockViewPtr, getFocusedRunBankIDToAdd())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFocusedRunNumberToAdd())
        .Times(1)
        .WillOnce(Return(123));
    EXPECT_CALL(*m_mockModelPtr, addFocusedRun(123, 1, ws));

    const std::vector<std::pair<int, size_t>> workspaceLabels(
        {std::make_pair(123, 1)});
    EXPECT_CALL(*m_mockModelPtr, getAllWorkspaceLabels())
        .Times(1)
        .WillOnce(Return(workspaceLabels));
    EXPECT_CALL(*m_mockViewPtr, updateRunList(workspaceLabels)).Times(1);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::AddFocusedRun);
    assertMocksUsedCorrectly();
  }

  void test_addFittedPeaks() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksWorkspaceToAdd())
        .Times(1)
        .WillOnce(Return(ws));
    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksBankIDToAdd())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksRunNumberToAdd())
        .Times(1)
        .WillOnce(Return(123));
    EXPECT_CALL(*m_mockModelPtr, addFittedPeaks(123, 1, ws));

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::AddFittedPeaks);
    assertMocksUsedCorrectly();
  }

  void test_getFittedPeaksValid() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksBankIDToReturn())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksRunNumberToReturn())
        .Times(1)
        .WillOnce(Return(123));

    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockModelPtr, getFittedPeaks(123, 1))
        .Times(1)
        .WillOnce(Return(boost::make_optional<API::MatrixWorkspace_sptr>(ws)));
    EXPECT_CALL(*m_mockViewPtr, setFittedPeaksWorkspaceToReturn(ws));
    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFittedPeaks);

    assertMocksUsedCorrectly();
  }

  void test_getFittedPeaksInvalid() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksBankIDToReturn())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFittedPeaksRunNumberToReturn())
        .Times(1)
        .WillOnce(Return(123));

    EXPECT_CALL(*m_mockModelPtr, getFittedPeaks(123, 1))
        .Times(1)
        .WillOnce(Return(boost::none));

    EXPECT_CALL(*m_mockViewPtr,
                userError("Invalid fitted peaks run identifier",
                          "Could not find a fitted peaks workspace with run "
                          "number 123 and bank ID 1. Please contact the "
                          "development team with this message"))
        .Times(1);
    EXPECT_CALL(*m_mockViewPtr, setFittedPeaksWorkspaceToReturn(testing::_))
        .Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFittedPeaks);

    assertMocksUsedCorrectly();
  }

  void test_getFocusedRunValid() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getFocusedRunBankIDToReturn())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFocusedRunNumberToReturn())
        .Times(1)
        .WillOnce(Return(123));

    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1))
        .Times(1)
        .WillOnce(Return(boost::make_optional<API::MatrixWorkspace_sptr>(ws)));
    EXPECT_CALL(*m_mockViewPtr, setFocusedRunWorkspaceToReturn(ws));
    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFocusedRun);

    assertMocksUsedCorrectly();
  }

  void test_getFocusedRunInvalid() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockViewPtr, getFocusedRunBankIDToReturn())
        .Times(1)
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mockViewPtr, getFocusedRunNumberToReturn())
        .Times(1)
        .WillOnce(Return(123));

    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1))
        .Times(1)
        .WillOnce(Return(boost::none));

    EXPECT_CALL(*m_mockViewPtr,
                userError("Invalid focused run identifier",
                          "Could not find a focused run with run "
                          "number 123 and bank ID 1. Please contact the "
                          "development team with this message"))
        .Times(1);
    EXPECT_CALL(*m_mockViewPtr, setFocusedRunWorkspaceToReturn(testing::_))
        .Times(0);

    presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFocusedRun);

    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffMultiRunFittingWidgetModel *m_mockModelPtr;
  MockEnggDiffMultiRunFittingWidgetView *m_mockViewPtr;

  std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> setUpPresenter() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetModel>>();
    m_mockModelPtr = mockModel.get();

    m_mockViewPtr =
        new testing::NiceMock<MockEnggDiffMultiRunFittingWidgetView>();

    std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> pres_uptr(
        new EnggDiffMultiRunFittingWidgetPresenter(std::move(mockModel),
                                                   m_mockViewPtr));
    return pres_uptr;
  }

  void assertMocksUsedCorrectly() {
    if (m_mockViewPtr) {
      delete m_mockViewPtr;
    }
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModelPtr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockViewPtr));
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

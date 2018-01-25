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

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

  void test_getFocsedRun() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModelPtr, getFocusedRun(123, 1)).Times(1);

    presenter->getFocusedRun(123, 1);
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

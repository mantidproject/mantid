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

    const RunLabel runLabel(123, 1);
    EXPECT_CALL(*m_mockModel, addFittedPeaks(runLabel, ws)).Times(1);

    presenter->addFittedPeaks(runLabel, ws);
    assertMocksUsedCorrectly();
  }

  void test_focusedRunIsAddedToModel() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    const RunLabel runLabel(123, 1);

    EXPECT_CALL(*m_mockModel, addFocusedRun(runLabel, ws)).Times(1);

    const std::vector<RunLabel> workspaceLabels({runLabel});
    EXPECT_CALL(*m_mockModel, getAllWorkspaceLabels())
        .Times(1)
        .WillOnce(Return(workspaceLabels));

    presenter->addFocusedRun(runLabel, ws);
    assertMocksUsedCorrectly();
  }

  void test_loadRunUpdatesView() {
    auto presenter = setUpPresenter();
    const API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    const RunLabel runLabel(123, 1);
    const std::vector<RunLabel> workspaceLabels({runLabel});
    ON_CALL(*m_mockModel, getAllWorkspaceLabels())
        .WillByDefault(Return(workspaceLabels));
    EXPECT_CALL(*m_mockView, updateRunList(workspaceLabels));

    presenter->addFocusedRun(runLabel, ws);
    assertMocksUsedCorrectly();
  }

  void test_getFittedPeaks() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel(123, 1);
    EXPECT_CALL(*m_mockModel, getFittedPeaks(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));

    presenter->getFittedPeaks(runLabel);
    assertMocksUsedCorrectly();
  }

  void test_getFocusedRun() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel(123, 1);
    EXPECT_CALL(*m_mockModel, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));

    presenter->getFocusedRun(runLabel);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffMultiRunFittingWidgetModel *m_mockModel;
  MockEnggDiffMultiRunFittingWidgetView *m_mockView;

  std::unique_ptr<EnggDiffMultiRunFittingWidgetPresenter> setUpPresenter() {
    auto mockModel_uptr = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetModel>>();
    m_mockModel = mockModel_uptr.get();

    auto mockView_uptr = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetView>>();
    m_mockView = mockView_uptr.get();

    return Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetPresenter>(
        std::move(mockModel_uptr), std::move(mockView_uptr));
  }

  void assertMocksUsedCorrectly() {
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModel));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockView));
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERTEST_H_

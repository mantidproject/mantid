#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASFittingModelMock.h"
#include "EnggDiffGSASFittingViewMock.h"

#include "MantidKernel/make_unique.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;

class EnggDiffGSASFittingPresenterTest : public CxxTest::TestSuite {

public:
  void test_loadValidFile() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const auto filename = "Valid filename";

    EXPECT_CALL(*mockView_ptr, getFocusedFileName())
        .Times(1)
        .WillOnce(Return(filename));
    EXPECT_CALL(*mockModel_ptr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Return(""));

    const std::vector<std::pair<int, size_t>> runLabels(
        {std::make_pair(123, 1)});

    EXPECT_CALL(*mockModel_ptr, getRunLabels())
        .Times(1)
        .WillOnce(Return(runLabels));
    EXPECT_CALL(*mockView_ptr, updateRunList(runLabels)).Times(1);

    EXPECT_CALL(*mockView_ptr, userWarning(testing::_)).Times(0);

    pres.notify(IEnggDiffGSASFittingPresenter::LoadRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_loadInvalidFile() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const auto filename = "Invalid filename";

    EXPECT_CALL(*mockView_ptr, getFocusedFileName())
        .Times(1)
        .WillOnce(Return(filename));

    const auto warning = "File not found";
    EXPECT_CALL(*mockModel_ptr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Return(warning));

    EXPECT_CALL(*mockModel_ptr, getRunLabels()).Times(0);

    EXPECT_CALL(*mockView_ptr, userWarning(warning)).Times(1);

    pres.notify(IEnggDiffGSASFittingPresenter::LoadRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }
};

#endif

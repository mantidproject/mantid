#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONPRESENTERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffCalibrationPresenter.h"
#include "../EnggDiffraction/EnggDiffUserSettings.h"
#include "EnggDiffCalibrationModelMock.h"
#include "EnggDiffCalibrationViewMock.h"

#include "MantidKernel/make_unique.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;

class EnggDiffCalibrationPresenterTest : public CxxTest::TestSuite {
public:
  void test_loadFailsWithNoInput() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getInputFilename()).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_mockView,
                userWarning("Invalid calibration file", "No file selected"));
    EXPECT_CALL(*m_mockModel, parseCalibrationFile(testing::_)).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::LoadCalibration);
    assertMocksUsedCorrectly();
  }

  void test_loadFailsWithInvalidFilename() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockView, getInputFilename())
        .WillOnce(
            Return(boost::make_optional<std::string>("invalid_name.prm")));
    EXPECT_CALL(*m_mockView,
                userWarning("Invalid calibration filename", testing::_));
    EXPECT_CALL(*m_mockModel, parseCalibrationFile(testing::_)).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::LoadCalibration);
    assertMocksUsedCorrectly();
  }

  void test_loadFailsWithIncorrectInstrument() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockView, getInputFilename())
        .WillOnce(
            Return(boost::make_optional<std::string>("OTHERINST_123_456.prm")));
    EXPECT_CALL(*m_mockView,
                userWarning("Invalid calibration filename", testing::_));
    EXPECT_CALL(*m_mockModel, parseCalibrationFile(testing::_)).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::LoadCalibration);
    assertMocksUsedCorrectly();
  }

  void test_loadValidFileUpdatesViewAndModel() {
    auto presenter = setUpPresenter();
    const auto filename = "TESTINST_123_456.prm";
    const GSASCalibrationParameters calibParams(1, 2, 3, 4);
    const std::vector<GSASCalibrationParameters> calibParamsVec({calibParams});

    EXPECT_CALL(*m_mockView, getInputFilename())
        .WillOnce(Return(boost::make_optional<std::string>(filename)));
    EXPECT_CALL(*m_mockView, userWarning(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockModel, parseCalibrationFile(filename))
        .WillOnce(Return(calibParamsVec));
    EXPECT_CALL(*m_mockView, displayLoadedVanadiumRunNumber("123"));
    EXPECT_CALL(*m_mockView, displayLoadedCeriaRunNumber("456"));
    EXPECT_CALL(*m_mockModel, setCalibrationParams(calibParamsVec));

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::LoadCalibration);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffCalibrationModel *m_mockModel;
  MockEnggDiffCalibrationView *m_mockView;

  const std::string INST_NAME = "TESTINST";

  void assertMocksUsedCorrectly() {
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModel));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockView));
  }

  std::unique_ptr<EnggDiffCalibrationPresenter> setUpPresenter() {
    auto mockModel_uptr = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffCalibrationModel>>();
    m_mockModel = mockModel_uptr.get();

    auto mockView_sptr =
        boost::make_shared<testing::NiceMock<MockEnggDiffCalibrationView>>();
    m_mockView = mockView_sptr.get();

    auto userSettings = boost::make_shared<EnggDiffUserSettings>(INST_NAME);

    return Mantid::Kernel::make_unique<EnggDiffCalibrationPresenter>(
        std::move(mockModel_uptr), mockView_sptr, userSettings);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONPRESENTERTEST_H_

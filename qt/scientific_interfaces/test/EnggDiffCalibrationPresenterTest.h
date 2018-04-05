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
using testing::Throw;

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
    const auto filename = "/path/to/TESTINST_123_456.prm";
    const GSASCalibrationParameters calibParams(1, 2, 3, 4, "123", "456",
                                                filename);
    const std::vector<GSASCalibrationParameters> calibParamsVec({calibParams});

    EXPECT_CALL(*m_mockView, getInputFilename())
        .WillOnce(Return(boost::make_optional<std::string>(filename)));
    EXPECT_CALL(*m_mockView, userWarning(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockModel, parseCalibrationFile(filename))
        .WillOnce(Return(calibParamsVec));
    EXPECT_CALL(*m_mockView, setCurrentCalibVanadiumRunNumber("123"));
    EXPECT_CALL(*m_mockView, setCurrentCalibCeriaRunNumber("456"));
    EXPECT_CALL(*m_mockModel, setCalibrationParams(calibParamsVec));

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::LoadCalibration);
    assertMocksUsedCorrectly();
  }

  void test_createCalibRequiresVanadium() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getNewCalibVanadiumInput()).WillOnce(Return(""));
    EXPECT_CALL(
        *m_mockView,
        userWarning("No vanadium entered",
                    "Please enter a vanadium run number to calibrate against"));
    EXPECT_CALL(*m_mockView, getNewCalibCeriaInput()).Times(0);

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_createCalibRequiresCeria() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getNewCalibCeriaInput()).WillOnce(Return(""));
    EXPECT_CALL(
        *m_mockView,
        userWarning("No ceria entered",
                    "Please enter a ceria run number to calibrate against"));
    EXPECT_CALL(*m_mockModel, createCalibration(testing::_, testing::_))
        .Times(0);

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_newCalibInputCanBeRunNumbers() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModel,
                createCalibration(INST_NAME + VAN_NUM, INST_NAME + CERIA_NUM));

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_newCalibInputCanBePaths() {
    auto presenter = setUpPresenter();
    const auto vanFile = "/path/to/van/file";
    const auto ceriaFile = "/path/to/ceria/file";

    EXPECT_CALL(*m_mockView, getNewCalibVanadiumInput())
        .WillOnce(Return(vanFile));
    EXPECT_CALL(*m_mockView, getNewCalibCeriaInput())
        .WillOnce(Return(ceriaFile));

    EXPECT_CALL(*m_mockModel, createCalibration(vanFile, ceriaFile));

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_createCalibHandlesErrorInModel() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModel, createCalibration(testing::_, testing::_))
        .WillOnce(Throw(std::runtime_error("Failure reason")));
    EXPECT_CALL(*m_mockView,
                userWarning("Calibration failed", "Failure reason"));
    EXPECT_CALL(*m_mockModel, setCalibrationParams(testing::_)).Times(0);

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_successfulCalibUpdatesModelAndView() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockModel, setCalibrationParams(CALIB_PARAMS));
    EXPECT_CALL(*m_mockView, setCurrentCalibVanadiumRunNumber(VAN_NUM));
    EXPECT_CALL(*m_mockView, setCurrentCalibCeriaRunNumber(CERIA_NUM));
    EXPECT_CALL(*m_mockView, setCalibFilePath(CALIB_PARAMS[0].filePath));
    EXPECT_CALL(*m_mockView, userWarning(testing::_, testing::_)).Times(0);

    presenter->notify(IEnggDiffCalibrationPresenter::Notification::Calibrate);
    assertMocksUsedCorrectly();
  }

  void test_createCalibCroppedRequiresVanadium() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getNewCalibVanadiumInput()).WillOnce(Return(""));
    EXPECT_CALL(
        *m_mockView,
        userWarning("No vanadium entered",
                    "Please enter a vanadium run number to calibrate against"));
    EXPECT_CALL(*m_mockView, getNewCalibCeriaInput()).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

  void test_createCalibCroppedRequiresCeria() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getNewCalibCeriaInput()).WillOnce(Return(""));
    EXPECT_CALL(
        *m_mockView,
        userWarning("No ceria entered",
                    "Please enter a ceria run number to calibrate against"));
    EXPECT_CALL(*m_mockModel, createCalibration(testing::_, testing::_))
        .Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

  void test_createCalibCroppedUpdatesViewAndModel() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getCalibCropType())
        .WillOnce(Return(CalibCropType::NORTH_BANK));
    EXPECT_CALL(*m_mockModel, createCalibrationByBank(1, INST_NAME + VAN_NUM,
                                                      INST_NAME + CERIA_NUM))
        .WillOnce(Return(CALIB_PARAMS));

    EXPECT_CALL(*m_mockModel, setCalibrationParams(CALIB_PARAMS));
    EXPECT_CALL(*m_mockView, setCurrentCalibVanadiumRunNumber(VAN_NUM));
    EXPECT_CALL(*m_mockView, setCurrentCalibCeriaRunNumber(CERIA_NUM));
    EXPECT_CALL(*m_mockView, setCalibFilePath(CALIB_PARAMS[0].filePath));
    EXPECT_CALL(*m_mockView, userWarning(testing::_, testing::_)).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

  void test_createCalibCroppedHandlesErrorInModel() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getCalibCropType())
        .WillOnce(Return(CalibCropType::SOUTH_BANK));
    EXPECT_CALL(*m_mockModel, createCalibrationByBank(2, INST_NAME + VAN_NUM,
                                                      INST_NAME + CERIA_NUM))
        .WillOnce(Throw(std::runtime_error("Failure reason")));
    EXPECT_CALL(*m_mockView,
                userWarning("Calibration failed", "Failure reason"));

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

  void test_createCalibSpecNumsRequiresSpecNums() {
    auto presenter = setUpPresenter();

    ON_CALL(*m_mockView, getCalibCropType())
        .WillByDefault(Return(CalibCropType::SPEC_NUMS));
    EXPECT_CALL(*m_mockView, getSpectrumNumbers()).WillOnce(Return(""));
    EXPECT_CALL(
        *m_mockView,
        userWarning(
            "No spectrum numbers",
            "Please enter a set of spectrum numbers to use for focusing"));
    EXPECT_CALL(*m_mockView, getCustomBankName()).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

  void test_createCalibSpecNumsUpdatesViewAndModel() {
    auto presenter = setUpPresenter();

    EXPECT_CALL(*m_mockView, getCalibCropType())
        .WillOnce(Return(CalibCropType::SPEC_NUMS));

    ON_CALL(*m_mockView, getSpectrumNumbers()).WillByDefault(Return("1,2,3"));
    ON_CALL(*m_mockView, getCustomBankName()).WillByDefault(Return("cropped"));

    EXPECT_CALL(*m_mockModel, createCalibrationBySpectra("1,2,3", "cropped",
                                                         INST_NAME + VAN_NUM,
                                                         INST_NAME + CERIA_NUM))
        .WillOnce(Return(CALIB_PARAMS));

    EXPECT_CALL(*m_mockModel, setCalibrationParams(CALIB_PARAMS));
    EXPECT_CALL(*m_mockView, setCurrentCalibVanadiumRunNumber(VAN_NUM));
    EXPECT_CALL(*m_mockView, setCurrentCalibCeriaRunNumber(CERIA_NUM));
    EXPECT_CALL(*m_mockView, setCalibFilePath(CALIB_PARAMS[0].filePath));
    EXPECT_CALL(*m_mockView, userWarning(testing::_, testing::_)).Times(0);

    presenter->notify(
        IEnggDiffCalibrationPresenter::Notification::CalibrateCropped);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffCalibrationModel *m_mockModel;
  MockEnggDiffCalibrationView *m_mockView;

  const std::string INST_NAME = "TESTINST";
  const std::string VAN_NUM = "123";
  const std::string CERIA_NUM = "456";
  const std::vector<GSASCalibrationParameters> CALIB_PARAMS = {
      GSASCalibrationParameters(1, 2, 3, 4, VAN_NUM, CERIA_NUM,
                                "/path/to/calib/file")};

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

    ON_CALL(*m_mockView, getNewCalibVanadiumInput())
        .WillByDefault(Return(VAN_NUM));
    ON_CALL(*m_mockView, getNewCalibCeriaInput())
        .WillByDefault(Return(CERIA_NUM));

    ON_CALL(*m_mockModel, createCalibration(testing::_, testing::_))
        .WillByDefault(Return(CALIB_PARAMS));

    auto userSettings = boost::make_shared<EnggDiffUserSettings>(INST_NAME);

    return Mantid::Kernel::make_unique<EnggDiffCalibrationPresenter>(
        std::move(mockModel_uptr), mockView_sptr, userSettings);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBRATIONPRESENTERTEST_H_

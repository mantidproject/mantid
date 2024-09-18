// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "../Muon/ALCDataLoadingPresenter.h"
#include "../Muon/IALCDataLoadingView.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &out,
                                                    std::optional<std::pair<double, double>> const &maybe) {
  if (maybe)
    out << maybe->first << ", " << maybe->second;
  return out;
}
} // namespace boost

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALCDataLoadingView : public IALCDataLoadingView {
  // XXX: A workaround, needed because of the way the comma is treated in a
  // macro
  using PAIR_OF_DOUBLES = std::pair<double, double>;

public:
  MOCK_CONST_METHOD0(getInstrument, std::string());
  MOCK_CONST_METHOD0(getPath, std::string());
  MOCK_CONST_METHOD0(log, std::string());
  MOCK_CONST_METHOD0(function, std::string());
  MOCK_CONST_METHOD0(calculationType, std::string());
  MOCK_CONST_METHOD0(timeRange, std::optional<PAIR_OF_DOUBLES>());
  MOCK_CONST_METHOD0(deadTimeType, std::string());
  MOCK_CONST_METHOD0(deadTimeFile, std::string());
  MOCK_CONST_METHOD0(detectorGroupingType, std::string());
  MOCK_CONST_METHOD0(getForwardGrouping, std::string());
  MOCK_CONST_METHOD0(getBackwardGrouping, std::string());
  MOCK_CONST_METHOD0(redPeriod, std::string());
  MOCK_CONST_METHOD0(greenPeriod, std::string());
  MOCK_CONST_METHOD0(subtractIsChecked, bool());
  MOCK_CONST_METHOD0(getRunsText, std::string());
  MOCK_CONST_METHOD0(getRunsFirstRunText, std::string());
  MOCK_CONST_METHOD0(getAlphaValue, std::string());
  MOCK_CONST_METHOD0(isAlphaEnabled, bool());

  MOCK_METHOD1(setFileExtensions, void(const std::vector<std::string> &extensions));
  MOCK_METHOD0(initialize, void());
  MOCK_METHOD2(setDataCurve, void(MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex));
  MOCK_METHOD1(displayError, void(const std::string &));
  MOCK_METHOD1(displayWarning, bool(const std::string &));
  MOCK_METHOD1(setAvailableLogs, void(const std::vector<std::string> &));
  MOCK_METHOD1(setAvailablePeriods, void(const std::vector<std::string> &));
  MOCK_METHOD2(setTimeLimits, void(double, double));
  MOCK_METHOD2(setTimeRange, void(double, double));
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_METHOD0(help, void());
  MOCK_METHOD0(setAvailableInfoToEmpty, void());
  MOCK_METHOD0(initInstruments, void());
  MOCK_METHOD1(instrumentChanged, void(QString));
  MOCK_METHOD1(enableLoad, void(bool));
  MOCK_METHOD1(setPath, void(const std::string &));
  MOCK_METHOD1(enableRunsAutoAdd, void(bool));
  MOCK_METHOD1(setInstrument, void(const std::string &));
  MOCK_METHOD0(getRunsError, std::string());
  MOCK_METHOD0(getFiles, std::vector<std::string>());
  MOCK_METHOD0(getFirstFile, std::string());
  MOCK_METHOD2(setLoadStatus, void(const std::string &, const std::string &));
  MOCK_METHOD1(runsAutoAddToggled, void(bool));
  MOCK_METHOD1(setRunsTextWithoutSearch, void(const std::string &));
  MOCK_METHOD1(toggleRunsAutoAdd, void(const bool));
  MOCK_METHOD1(enableAlpha, void(const bool));
  MOCK_METHOD1(setAlphaValue, void(const std::string &));
  MOCK_METHOD1(showAlphaMessage, void(const bool));

  // Some dummy signals
  void emitRunsEditingSignal() { emit runsEditingSignal(); }
  void changeRuns() { emit runsEditingFinishedSignal(); }
  void foundRuns() { emit runsFoundSignal(); }
  void requestLoading() { emit loadRequested(); }
};

MATCHER_P4(WorkspaceX, i, j, value, delta, "") {
  if (fabs(arg->x(i)[j] - value) < delta) {
    return true;
  } else {
    std::cout << "WorkspaceX(" << i << ", " << j << ") = " << arg->x(i)[j] << " exp = " << value << "\n";
    return false;
  }
}
MATCHER_P4(WorkspaceY, i, j, value, delta, "") {
  if (fabs(arg->y(i)[j] - value) < delta) {
    return true;
  } else {
    std::cout << "WorkspaceY(" << i << ", " << j << ") = " << arg->y(i)[j] << " exp = " << value << "\n";
    return false;
  }
}

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ALCDataLoadingPresenterTest : public CxxTest::TestSuite {
  MockALCDataLoadingView *m_view;
  ALCDataLoadingPresenter *m_presenter;

  std::string loadingString = std::string("Loading MUSR15189,15191-92");
  std::string loadedString = std::string("Successfully loaded MUSR15189,15191-92");
  std::string foundString = std::string("Successfully found MUSR15189,15191-92");

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCDataLoadingPresenterTest *createSuite() { return new ALCDataLoadingPresenterTest(); }
  static void destroySuite(ALCDataLoadingPresenterTest *suite) { delete suite; }

  ALCDataLoadingPresenterTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    m_view = new NiceMock<MockALCDataLoadingView>();
    m_presenter = new ALCDataLoadingPresenter(m_view);
    m_presenter->initialize();

    // Set some valid default return values for the view mock object getters
    std::vector<std::string> defaultFiles = {"MUSR00015189.nxs", "MUSR00015191.nxs", "MUSR00015192.nxs"};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(defaultFiles));
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return(defaultFiles.front()));
    ON_CALL(*m_view, getRunsText()).WillByDefault(Return("15189,15191-92"));
    ON_CALL(*m_view, getRunsError()).WillByDefault(Return(std::string{}));
    ON_CALL(*m_view, getRunsFirstRunText()).WillByDefault(Return(std::string{}));
    ON_CALL(*m_view, getInstrument()).WillByDefault(Return("MUSR"));
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Integral"));
    ON_CALL(*m_view, log()).WillByDefault(Return("sample_magn_field"));
    ON_CALL(*m_view, function()).WillByDefault(Return("Last"));
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(std::make_optional(std::make_pair(-6.0, 32.0))));
    // Add range for integration
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("None"));
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Auto"));
    ON_CALL(*m_view, redPeriod()).WillByDefault(Return("1"));
    ON_CALL(*m_view, subtractIsChecked()).WillByDefault(Return(false));
    ON_CALL(*m_view, getAlphaValue()).WillByDefault(Return("1.0"));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    delete m_presenter;
    delete m_view;
  }

  void test_initialize() {
    MockALCDataLoadingView view;
    ALCDataLoadingPresenter presenter(&view);
    EXPECT_CALL(view, initialize());
    std::vector<std::string> expected{".nxs", ".nxs_v2", ".bin"};
    EXPECT_CALL(view, setFileExtensions(expected));
    presenter.initialize();
  }

  void test_setDataThrowsWithNullData() {
    MockALCDataLoadingView view;
    ALCDataLoadingPresenter presenter(&view);
    TS_ASSERT_THROWS_EQUALS(presenter.setData(nullptr), const std::invalid_argument &e, std::string(e.what()),
                            "Cannot load an empty workspace");
  }

  void test_defaultLoad() {
    InSequence s;

    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, disableAll());
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1370, 1E-8),
                                            WorkspaceX(0, 2, 1380, 1E-8), WorkspaceY(0, 0, 0.150, 1E-3),
                                            WorkspaceY(0, 1, 0.128, 1E-3), WorkspaceY(0, 2, 0.109, 1E-3)),
                                      0));
    EXPECT_CALL(*m_view, enableAll());
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_load_differential() {
    // Change to differential calculation type
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Differential"));

    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 3.00349, 1E-3), WorkspaceY(0, 1, 2.47935, 1E-3),
                                            WorkspaceY(0, 2, 1.85123, 1E-3)),
                                      0));

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_load_timeLimits() {
    // Set time limit
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(std::make_optional(std::make_pair(5.0, 10.0))));

    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 0.137, 1E-3), WorkspaceY(0, 1, 0.111, 1E-3),
                                            WorkspaceY(0, 2, 0.109, 1E-3)),
                                      0));

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_updateAvailableInfo() {
    // Test time limits
    auto timeRange = std::make_pair<double, double>(0.0, 0.0);
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(std::make_optional(timeRange)));

    EXPECT_CALL(*m_view, getFirstFile()).WillRepeatedly(Return("MUSR00015189.nxs"));
    // Test logs
    EXPECT_CALL(*m_view, setAvailableLogs(AllOf(Property(&std::vector<std::string>::size, 46), Contains("run_number"),
                                                Contains("sample_magn_field"), Contains("Field_Danfysik"))))
        .Times(1);
    // Test periods
    EXPECT_CALL(*m_view,
                setAvailablePeriods(AllOf(Property(&std::vector<std::string>::size, 2), Contains("1"), Contains("2"))))
        .Times(1);
    EXPECT_CALL(*m_view, setTimeLimits(Le(0.107), Ge(31.44))).Times(1);

    m_view->foundRuns();
  }

  void test_updateAvailableInfo_NotFirstRun() {
    // Test time limits
    auto timeRange = std::make_pair<double, double>(0.1, 10.0); // not the first run loaded
    ON_CALL(*m_view, timeRange()).WillByDefault(Return(std::make_optional(timeRange)));

    EXPECT_CALL(*m_view, getFirstFile()).WillRepeatedly(Return("MUSR00015189.nxs"));
    // Test logs
    EXPECT_CALL(*m_view, setAvailableLogs(AllOf(Property(&std::vector<std::string>::size, 46), Contains("run_number"),
                                                Contains("sample_magn_field"), Contains("Field_Danfysik"))))
        .Times(1);
    // Test periods
    EXPECT_CALL(*m_view,
                setAvailablePeriods(AllOf(Property(&std::vector<std::string>::size, 2), Contains("1"), Contains("2"))))
        .Times(1);
    EXPECT_CALL(*m_view, setTimeLimits(_, _)).Times(0); // shouldn't reset time limits

    m_view->foundRuns();
  }

  void test_badCustomGroupingOutofRange() {
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Custom"));
    ON_CALL(*m_view, getForwardGrouping()).WillByDefault(Return("1-48"));
    // Too many detectors (MUSR has only 64)
    ON_CALL(*m_view, getBackwardGrouping()).WillByDefault(Return("49-96"));

    EXPECT_CALL(*m_view, enableLoad(true)).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(foundString, "green")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(false)).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);

    m_view->foundRuns();
    m_view->requestLoading();
  }

  void test_badCustomGroupingLetter() {
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Custom"));
    ON_CALL(*m_view, getForwardGrouping()).WillByDefault(Return("1,2"));
    ON_CALL(*m_view, getBackwardGrouping()).WillByDefault(Return("3,a"));

    EXPECT_CALL(*m_view, enableLoad(true)).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(foundString, "green")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(false)).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);

    m_view->foundRuns();
    m_view->requestLoading();
  }

  void test_badCustomGroupingDecimal() {
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Custom"));
    ON_CALL(*m_view, getForwardGrouping()).WillByDefault(Return("1.2,2"));
    // Too many detectors (MUSR has only 64)
    ON_CALL(*m_view, getBackwardGrouping()).WillByDefault(Return("3,4"));

    EXPECT_CALL(*m_view, enableLoad(true)).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(foundString, "green")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(false)).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);

    m_view->foundRuns();
    m_view->requestLoading();
  }

  void test_updateAvailableLogs_invalidFirstRun() {
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return(""));

    EXPECT_CALL(*m_view, setAvailableInfoToEmpty()).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);

    m_view->foundRuns();
  }

  void test_updateAvailableLogs_unsupportedFirstRun() {
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return("LOQ49886.nxs")); // XXX: not a Muon file

    EXPECT_CALL(*m_view, setAvailableInfoToEmpty()).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);

    m_view->foundRuns();
  }

  void test_load_nonExistentFile() {
    std::vector<std::string> nonExistent{"non-existent-file"};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(nonExistent));

    EXPECT_CALL(*m_view, setDataCurve(_, _)).Times(0);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(false)).Times(1);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);

    m_view->requestLoading();
  }

  void test_load_empty_files() {
    std::vector<std::string> emptyVec{};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(emptyVec));

    EXPECT_CALL(*m_view, setDataCurve(_, _)).Times(0);
    EXPECT_CALL(*m_view, setLoadStatus("Error", "red")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(false)).Times(1);
    EXPECT_CALL(*m_view, displayError("The list of files to load is empty")).Times(1);

    m_view->requestLoading();
  }

  void test_correctionsFromDataFile() {
    // Change dead time correction type
    // Test results with corrections from run data
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("FromRunData"));

    EXPECT_CALL(*m_view, deadTimeType()).Times(2);
    EXPECT_CALL(*m_view, deadTimeFile()).Times(0);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 0.151202, 1E-3), WorkspaceY(0, 1, 0.129347, 1E-3),
                                            WorkspaceY(0, 2, 0.109803, 1E-3)),
                                      0));
    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_correctionsFromCustomFile() {
    // Change dead time correction type
    // Test only expected number of calls, alg will fail as no file specified
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("FromSpecifiedFile"));

    EXPECT_CALL(*m_view, deadTimeType()).Times(2);
    EXPECT_CALL(*m_view, deadTimeFile()).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_customGrouping() {
    // Change grouping type to 'Custom'
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Custom"));
    // Set grouping, the same as the default
    ON_CALL(*m_view, getForwardGrouping()).WillByDefault(Return("33-64"));
    ON_CALL(*m_view, getBackwardGrouping()).WillByDefault(Return("1-32"));

    EXPECT_CALL(*m_view, getForwardGrouping()).Times(2);
    EXPECT_CALL(*m_view, getBackwardGrouping()).Times(2);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, enableLoad(true)).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(foundString, "green")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1370, 1E-8),
                                            WorkspaceX(0, 2, 1380, 1E-8), WorkspaceY(0, 0, 0.150, 1E-3),
                                            WorkspaceY(0, 1, 0.128, 1E-3), WorkspaceY(0, 2, 0.109, 1E-3)),
                                      0));

    m_view->foundRuns();
    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_customPeriods() {
    // Change red period to 2
    // Change green period to 1
    // Check Subtract, greenPeriod() should be called once
    ON_CALL(*m_view, subtractIsChecked()).WillByDefault(Return(true));
    ON_CALL(*m_view, redPeriod()).WillByDefault(Return("2"));
    ON_CALL(*m_view, greenPeriod()).WillByDefault(Return("1"));

    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, greenPeriod()).Times(1);
    // Check results
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1370, 1E-8),
                                            WorkspaceX(0, 2, 1380, 1E-8), WorkspaceY(0, 0, 0.012884, 1E-6),
                                            WorkspaceY(0, 1, 0.038717, 1E-6), WorkspaceY(0, 2, 0.054546, 1E-6)),
                                      0));
    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_logFunction() {
    ON_CALL(*m_view, function()).WillByDefault(Return("First"));
    ON_CALL(*m_view, log()).WillByDefault(Return("Field_Danfysik"));

    EXPECT_CALL(*m_view, getFiles()).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1364.520, 1E-3), WorkspaceX(0, 1, 1380.000, 1E-3),
                                            WorkspaceX(0, 2, 1398.090, 1E-3), WorkspaceY(0, 0, 0.12838, 1E-5),
                                            WorkspaceY(0, 1, 0.10900, 1E-5), WorkspaceY(0, 2, 0.15004, 1E-5)),
                                      0));

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_helpPage() {
    EXPECT_CALL(*m_view, help()).Times(1);
    m_view->help();
  }

  void test_warning_shows_and_press_yes() {
    // Ensure files greater than 200
    std::vector<std::string> files(201, "MUSR00015189.nxs");
    auto warningMessage = "You are attempting to load 201 runs, are you "
                          "sure you want to do this?";
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(files));
    ON_CALL(*m_view, displayWarning(warningMessage)).WillByDefault(Return(true));

    EXPECT_CALL(*m_view, getFiles()).Times(1);
    EXPECT_CALL(*m_view, displayWarning(warningMessage)).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadingString, "orange")).Times(1);
    EXPECT_CALL(*m_view, setLoadStatus(loadedString, "green")).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_warning_shows_and_press_no() {
    // Ensure files greater than 200
    std::vector<std::string> files(201, "MUSR00015189.nxs");
    auto warningMessage = "You are attempting to load 201 runs, are you "
                          "sure you want to do this?";
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(files));
    ON_CALL(*m_view, displayWarning(warningMessage)).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, getFiles()).Times(1);
    EXPECT_CALL(*m_view, displayWarning(warningMessage)).Times(1);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(0);

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_warning_does_not_show() {
    EXPECT_CALL(*m_view, getFiles()).Times(1);

    EXPECT_CALL(*m_view, displayWarning(StrNe(""))).Times(0);
    EXPECT_CALL(*m_view, enableRunsAutoAdd(true)).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_alpha_multi_period_data() {
    std::string multiPeriod = "MUSR00015189.nxs";
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return(multiPeriod));

    EXPECT_CALL(*m_view, enableAlpha(false)).Times(1);
    EXPECT_CALL(*m_view, showAlphaMessage(true)).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->foundRuns());
  }

  void test_alpha_single_period_data() {
    std::string singlePeriod = "EMU00019489.nxs";
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return(singlePeriod));

    EXPECT_CALL(*m_view, enableAlpha(true)).Times(1);
    EXPECT_CALL(*m_view, setAlphaValue(std::string{"1.0"})).Times(1);
    EXPECT_CALL(*m_view, showAlphaMessage(false)).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_view->foundRuns());

    // Reset ON_CALL for other tests
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return("MUSR00015189.nxs"));
  }

  void test_alpha_applied_correctly_single_period_data() {
    std::string singlePeriod = "EMU00019489.nxs";
    ON_CALL(*m_view, getFirstFile()).WillByDefault(Return(singlePeriod));
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(std::vector<std::string>{singlePeriod}));
    ON_CALL(*m_view, getAlphaValue()).WillByDefault(Return(std::string{"0.9"}));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 2000, 1E-3), WorkspaceY(0, 0, 0.29773, 1E-5)), 0));
    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_that_the_runsEditingSignal_will_disable_the_load_button() {
    EXPECT_CALL(*m_view, enableLoad(false)).Times(1);
    EXPECT_CALL(*m_view, setPath(std::string{})).Times(1);

    m_view->emitRunsEditingSignal();
  }

  void test_get_path_from_files_multiple_directories() {
    std::vector<std::string> files = {"path1/file.nxs", "path2/file.nxs"};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(files));
    EXPECT_CALL(*m_view, setPath(std::string{"Multiple Directories"})).Times(1);
    m_view->foundRuns();
  }

  void test_get_path_from_files_single_directory() {
    std::vector<std::string> files = {"path/file1.nxs", "path/file2.nxs"};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(files));
    EXPECT_CALL(*m_view, setPath(std::string{"path"})).Times(1);
    m_view->foundRuns();
  }

  void test_get_path_from_empty_files() {
    std::vector<std::string> files = {};
    ON_CALL(*m_view, getFiles()).WillByDefault(Return(files));
    EXPECT_CALL(*m_view, setPath(std::string{""})).Times(1);
    m_view->foundRuns();
  }
};

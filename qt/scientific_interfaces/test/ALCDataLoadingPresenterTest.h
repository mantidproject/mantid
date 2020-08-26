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
std::basic_ostream<CharType, CharTrait> &
operator<<(std::basic_ostream<CharType, CharTrait> &out,
           optional<std::pair<double, double>> const &maybe) {
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
  MOCK_CONST_METHOD0(firstRun, std::string());
  MOCK_CONST_METHOD0(lastRun, std::string());
  MOCK_CONST_METHOD0(getRuns, std::vector<std::string>());
  MOCK_CONST_METHOD0(log, std::string());
  MOCK_CONST_METHOD0(function, std::string());
  MOCK_CONST_METHOD0(calculationType, std::string());
  MOCK_CONST_METHOD0(timeRange, boost::optional<PAIR_OF_DOUBLES>());
  MOCK_CONST_METHOD0(deadTimeType, std::string());
  MOCK_CONST_METHOD0(deadTimeFile, std::string());
  MOCK_CONST_METHOD0(detectorGroupingType, std::string());
  MOCK_CONST_METHOD0(getForwardGrouping, std::string());
  MOCK_CONST_METHOD0(getBackwardGrouping, std::string());
  MOCK_CONST_METHOD0(redPeriod, std::string());
  MOCK_CONST_METHOD0(greenPeriod, std::string());
  MOCK_CONST_METHOD0(subtractIsChecked, bool());
  MOCK_METHOD1(setCurrentAutoRun, void(int));
  MOCK_METHOD0(updateRunsTextFromAuto, void());
  MOCK_CONST_METHOD0(getCurrentRunsText, std::string());
  MOCK_METHOD1(setRunsTextWithSearch, void(const QString &));
  MOCK_CONST_METHOD0(getRunsOldInput, std::string());
  MOCK_METHOD1(setRunsOldInput, void(const std::string &));

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD2(setDataCurve, void(MatrixWorkspace_sptr workspace,
                                  const std::size_t &workspaceIndex));
  MOCK_METHOD1(displayError, void(const std::string &));
  MOCK_METHOD1(setAvailableLogs, void(const std::vector<std::string> &));
  MOCK_METHOD1(setAvailablePeriods, void(const std::vector<std::string> &));
  MOCK_METHOD2(setTimeLimits, void(double, double));
  MOCK_METHOD2(setTimeRange, void(double, double));
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_METHOD0(help, void());
  MOCK_METHOD1(checkBoxAutoChanged, void(int));
  MOCK_METHOD1(setCurrentAutoFile, void(const std::string &));
  MOCK_METHOD0(handleFirstFileChanged, void());
  MOCK_METHOD1(extractRunNumber, int(const std::string &));

  void requestLoading() { emit loadRequested(); }
  void selectRuns() { emit runsSelected(); }
};

MATCHER_P4(WorkspaceX, i, j, value, delta, "") {
  return fabs(arg->x(i)[j] - value) < delta;
}
MATCHER_P4(WorkspaceY, i, j, value, delta, "") {
  return fabs(arg->y(i)[j] - value) < delta;
}

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ALCDataLoadingPresenterTest : public CxxTest::TestSuite {
  MockALCDataLoadingView *m_view;
  ALCDataLoadingPresenter *m_presenter;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCDataLoadingPresenterTest *createSuite() {
    return new ALCDataLoadingPresenterTest();
  }
  static void destroySuite(ALCDataLoadingPresenterTest *suite) { delete suite; }

  ALCDataLoadingPresenterTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    m_view = new NiceMock<MockALCDataLoadingView>();
    m_presenter = new ALCDataLoadingPresenter(m_view);
    m_presenter->initialize();

    std::vector<std::string> runs = {"MUSR00015189.nxs", "MUSR00015191.nxs",
                                     "MUSR00015192.nxs"};

    // Set some valid default return values for the view mock object getters
    ON_CALL(*m_view, firstRun()).WillByDefault(Return("MUSR00015189.nxs"));
    ON_CALL(*m_view, lastRun()).WillByDefault(Return("MUSR00015192.nxs"));
    ON_CALL(*m_view, getRuns()).WillByDefault(Return(runs));
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Integral"));
    ON_CALL(*m_view, log()).WillByDefault(Return("sample_magn_field"));
    ON_CALL(*m_view, function()).WillByDefault(Return("Last"));
    ON_CALL(*m_view, timeRange())
        .WillByDefault(
            Return(boost::make_optional(std::make_pair(-6.0, 32.0))));
    // Add range for integration
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("None"));
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Auto"));
    ON_CALL(*m_view, redPeriod()).WillByDefault(Return("1"));
    ON_CALL(*m_view, subtractIsChecked()).WillByDefault(Return(false));
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
    presenter.initialize();
  }

  void test_defaultLoad() {
    InSequence s;
    EXPECT_CALL(*m_view, disableAll());

    EXPECT_CALL(
        *m_view,
        setDataCurve(
            AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1370, 1E-8),
                  WorkspaceX(0, 2, 1380, 1E-8), WorkspaceY(0, 0, 0.150, 1E-3),
                  WorkspaceY(0, 1, 0.128, 1E-3), WorkspaceY(0, 2, 0.109, 1E-3)),
            0));

    EXPECT_CALL(*m_view, enableAll());

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_load_differential() {
    // Change to differential calculation type
    ON_CALL(*m_view, calculationType()).WillByDefault(Return("Differential"));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 3.00349, 1E-3),
                                            WorkspaceY(0, 1, 2.47935, 1E-3),
                                            WorkspaceY(0, 2, 1.85123, 1E-3)),
                                      0));

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_load_timeLimits() {
    // Set time limit
    ON_CALL(*m_view, timeRange())
        .WillByDefault(Return(boost::make_optional(std::make_pair(5.0, 10.0))));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 0.137, 1E-3),
                                            WorkspaceY(0, 1, 0.111, 1E-3),
                                            WorkspaceY(0, 2, 0.109, 1E-3)),
                                      0));

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

  void test_updateAvailableInfo() {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("MUSR00015189.nxs"));
    // Test logs
    EXPECT_CALL(*m_view,
                setAvailableLogs(
                    AllOf(Property(&std::vector<std::string>::size, 39),
                          Contains("run_number"), Contains("sample_magn_field"),
                          Contains("Field_Danfysik"))))
        .Times(1);
    // Test periods
    EXPECT_CALL(*m_view, setAvailablePeriods(
                             AllOf(Property(&std::vector<std::string>::size, 2),
                                   Contains("1"), Contains("2"))))
        .Times(1);
    // Test time limits
    auto timeRange = std::make_pair<double, double>(0.0, 0.0);
    ON_CALL(*m_view, timeRange())
        .WillByDefault(Return(boost::make_optional(timeRange)));
    EXPECT_CALL(*m_view, setTimeLimits(Le(0.107), Ge(31.44))).Times(1);
    m_view->selectRuns();
  }

  void test_updateAvailableInfo_NotFirstRun() {
    EXPECT_CALL(*m_view, firstRun()).WillRepeatedly(Return("MUSR00015189.nxs"));
    // Test logs
    EXPECT_CALL(*m_view,
                setAvailableLogs(
                    AllOf(Property(&std::vector<std::string>::size, 39),
                          Contains("run_number"), Contains("sample_magn_field"),
                          Contains("Field_Danfysik"))))
        .Times(1);
    // Test periods
    EXPECT_CALL(*m_view, setAvailablePeriods(
                             AllOf(Property(&std::vector<std::string>::size, 2),
                                   Contains("1"), Contains("2"))))
        .Times(1);
    // Test time limits
    auto timeRange =
        std::make_pair<double, double>(0.1, 10.0); // not the first run loaded
    ON_CALL(*m_view, timeRange())
        .WillByDefault(Return(boost::make_optional(timeRange)));
    EXPECT_CALL(*m_view, setTimeLimits(_, _))
        .Times(0); // shouldn't reset time limits
    m_view->selectRuns();
  }

  void test_badCustomGrouping() {
    ON_CALL(*m_view, detectorGroupingType()).WillByDefault(Return("Custom"));
    ON_CALL(*m_view, getForwardGrouping()).WillByDefault(Return("1-48"));
    // Too many detectors (MUSR has only 64)
    ON_CALL(*m_view, getBackwardGrouping()).WillByDefault(Return("49-96"));
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->selectRuns();
    m_view->requestLoading();
  }

  void test_updateAvailableLogs_invalidFirstRun() {
    ON_CALL(*m_view, firstRun()).WillByDefault(Return(""));
    EXPECT_CALL(*m_view,
                setAvailableLogs(ElementsAre())); // Empty array expected
    TS_ASSERT_THROWS_NOTHING(m_view->selectRuns());
  }

  void test_updateAvailableLogs_unsupportedFirstRun() {
    ON_CALL(*m_view, firstRun())
        .WillByDefault(Return("LOQ49886.nxs")); // XXX: not a Muon file
    EXPECT_CALL(*m_view,
                setAvailableLogs(ElementsAre())); // Empty array expected
    TS_ASSERT_THROWS_NOTHING(m_view->selectRuns());
  }

  void test_load_error() {
    // Set last run to one of the different instrument - should cause error
    // within algorithms exec
    std::vector<std::string> bad{"MUSR000015189.nxs", "EMU00006473.nxs"};
    ON_CALL(*m_view, getRuns()).WillByDefault(Return(bad));
    EXPECT_CALL(*m_view, setDataCurve(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }

  void test_load_invalidRun() {
    std::vector<std::string> emptyVec;
    ON_CALL(*m_view, getRuns()).WillByDefault(Return(emptyVec));
    EXPECT_CALL(*m_view, setDataCurve(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }

  void test_load_nonExistentFile() {
    std::vector<std::string> bad{"non-existent-file"};
    ON_CALL(*m_view,getRuns()).WillByDefault(Return(bad));
    EXPECT_CALL(*m_view, setDataCurve(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(StrNe(""))).Times(1);
    m_view->requestLoading();
  }

  void test_correctionsFromDataFile() {
    // Change dead time correction type
    // Test results with corrections from run data
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("FromRunData"));
    EXPECT_CALL(*m_view, deadTimeType()).Times(2);
    EXPECT_CALL(*m_view, deadTimeFile()).Times(0);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceY(0, 0, 0.151202, 1E-3),
                                            WorkspaceY(0, 1, 0.129347, 1E-3),
                                            WorkspaceY(0, 2, 0.109803, 1E-3)),
                                      0));
    m_view->requestLoading();
  }

  void test_correctionsFromCustomFile() {
    // Change dead time correction type
    // Test only expected number of calls
    ON_CALL(*m_view, deadTimeType()).WillByDefault(Return("FromSpecifiedFile"));
    EXPECT_CALL(*m_view, deadTimeType()).Times(2);
    EXPECT_CALL(*m_view, deadTimeFile()).Times(1);
    EXPECT_CALL(*m_view, enableAll()).Times(1);
    m_view->requestLoading();
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
    EXPECT_CALL(
        *m_view,
        setDataCurve(
            AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1370, 1E-8),
                  WorkspaceX(0, 2, 1380, 1E-8), WorkspaceY(0, 0, 0.150, 1E-3),
                  WorkspaceY(0, 1, 0.128, 1E-3), WorkspaceY(0, 2, 0.109, 1E-3)),
            0));
    m_view->selectRuns();
    m_view->requestLoading();
  }

  void test_customPeriods() {
    // Change red period to 2
    // Change green period to 1
    // Check Subtract, greenPeriod() should be called once
    ON_CALL(*m_view, subtractIsChecked()).WillByDefault(Return(true));
    ON_CALL(*m_view, redPeriod()).WillByDefault(Return("2"));
    ON_CALL(*m_view, greenPeriod()).WillByDefault(Return("1"));

    EXPECT_CALL(*m_view, greenPeriod()).Times(1);
    // Check results
    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1350, 1E-8),
                                            WorkspaceX(0, 1, 1370, 1E-8),
                                            WorkspaceX(0, 2, 1380, 1E-8),
                                            WorkspaceY(0, 0, 0.012884, 1E-6),
                                            WorkspaceY(0, 1, 0.038717, 1E-6),
                                            WorkspaceY(0, 2, 0.054546, 1E-6)),
                                      0));
    m_view->requestLoading();
  }

  void test_logFunction() {
    ON_CALL(*m_view, function()).WillByDefault(Return("First"));
    ON_CALL(*m_view, log()).WillByDefault(Return("Field_Danfysik"));

    EXPECT_CALL(*m_view, setDataCurve(AllOf(WorkspaceX(0, 0, 1364.520, 1E-3),
                                            WorkspaceX(0, 1, 1380.000, 1E-3),
                                            WorkspaceX(0, 2, 1398.090, 1E-3),
                                            WorkspaceY(0, 0, 0.12492, 1E-5),
                                            WorkspaceY(0, 1, 0.10353, 1E-5),
                                            WorkspaceY(0, 2, 0.14734, 1E-5)),
                                      0));

    m_view->requestLoading();
  }

  void test_helpPage() {
    EXPECT_CALL(*m_view, help()).Times(1);
    m_view->help();
  }
};

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DetermineSpinStateOrder.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/StringTokenizer.h"

#include <random>

using Mantid::Algorithms::DetermineSpinStateOrder;

std::vector<double> createFakeLogValues(size_t size, double mean) {
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution d(mean, 0.5);

  std::vector<double> logs;
  for (size_t i = 0; i < size; i++) {
    logs.push_back(d(gen));
  }
  return logs;
}

Mantid::API::WorkspaceGroup_sptr createWorkspaceGroupWithYValues(const std::vector<double> &yValues) {
  auto wsGroup = std::make_shared<Mantid::API::WorkspaceGroup>();
  for (const double yValue : yValues) {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 100, true, 100, yValue, 0, 0);
    wsGroup->addWorkspace(ws);
  }
  return wsGroup;
}

Mantid::API::WorkspaceGroup_sptr createWorkpsaceGroupWithYValuesAndFlipperLogs(const std::vector<double> &yValues,
                                                                               const std::vector<double> &logMeans,
                                                                               const std::string &instrumentName,
                                                                               const std::string &logName) {
  auto wsGroup = std::make_shared<Mantid::API::WorkspaceGroup>();
  Mantid::Types::Core::DateAndTime start("2025-06-25T10:08:00");
  std::vector<Mantid::Types::Core::DateAndTime> logTimes;
  for (double i = 0.0; i < logMeans.size(); i++) {
    logTimes.push_back(start + i);
  }
  for (double i = 0.0; i < yValues.size(); i++) {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 100, true, 100, yValues[i], 0, 0);
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws, false, false, instrumentName);
    auto spinFlipperLog = new Mantid::Kernel::TimeSeriesProperty<double>(
        logName, logTimes, createFakeLogValues(logTimes.size(), logMeans[i]));
    ws->mutableRun().addLogData(spinFlipperLog);
    wsGroup->addWorkspace(ws);
  }
  return wsGroup;
}

class DetermineSpinStateOrderTest : public CxxTest::TestSuite, DetermineSpinStateOrder {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetermineSpinStateOrderTest *createSuite() { return new DetermineSpinStateOrderTest(); }
  static void destroySuite(DetermineSpinStateOrderTest *suite) { delete suite; }

  void test_Init() {
    DetermineSpinStateOrder alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_validateInputs_inputWorkspaceSize() {
    auto wsGroupWithThreeItems = std::make_shared<Mantid::API::WorkspaceGroup>();
    for (int i = 0; i < 3; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10, false, false, true, "ZOOM");
      wsGroupWithThreeItems->addWorkspace(ws);
    }

    DetermineSpinStateOrder alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", wsGroupWithThreeItems);

    auto errors = alg.validateInputs();
    TS_ASSERT(!errors.empty())
    TS_ASSERT_EQUALS(errors["InputWorkspace"], "Input workspace group must have 4 entries (PA data)")
    WorkspaceCreationHelper::removeWS("three_items");
  }

  void test_validateInputs_unsupportedInstrument() {
    auto wsGroupOsiris = std::make_shared<Mantid::API::WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10, false, false, true, "OSIRIS");
      wsGroupOsiris->addWorkspace(ws);
    }

    DetermineSpinStateOrder alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", wsGroupOsiris);

    auto errors = alg.validateInputs();
    TS_ASSERT(!errors.empty())
    TS_ASSERT_EQUALS(errors["InputWorkspace"], "Sub workspaces must be data from either LARMOR or ZOOM")
  }

  void test_averageTransmission() {
    const std::vector<double> yValues = {10.0, 25.0, 80.0, 4.5};
    auto wsGroup = createWorkspaceGroupWithYValues(yValues);

    const double result = averageTransmition(wsGroup);

    const double averageYValue =
        std::accumulate(yValues.cbegin(), yValues.cend(), 0.0) / static_cast<double>(yValues.size());
    TS_ASSERT_EQUALS(result, averageYValue);
  }

  void heStateTest(const std::vector<double> &transmissionValues, const std::vector<char> &expectedSpinStates,
                   double flipperLogValue, const std::string &instrumentName, const std::string &sfLogName) {
    auto wsGroup = createWorkpsaceGroupWithYValuesAndFlipperLogs(
        transmissionValues, std::vector<double>(transmissionValues.size(), flipperLogValue), instrumentName, sfLogName);

    DetermineSpinStateOrder alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", wsGroup);
    alg.execute();

    const std::string result = alg.getPropertyValue("SpinStates");
    auto spinStates = Mantid::Kernel::StringTokenizer(result, ",").asVector();
    for (int i = 0; i < 4; ++i) {
      // get second char of the state string
      char rfState = spinStates[i][1];
      TS_ASSERT_EQUALS(rfState, expectedSpinStates[i]);
    }
  }

  void test_heStateWhenRfIsNegative() {
    const std::vector<double> transmissionValues = {10.0, 20.0, 80.0, 90.0};
    const std::vector<char> expectedSpinStates = {'+', '+', '-', '-'};
    heStateTest(transmissionValues, expectedSpinStates, 10.0, "LARMOR", "FlipperCurrent");
  }

  void test_heStateWhenRfIsPositive() {
    const std::vector<double> transmissionValues = {10.0, 20.0, 80.0, 90.0};
    const std::vector<char> expectedSpinStates = {'-', '-', '+', '+'};
    heStateTest(transmissionValues, expectedSpinStates, -10.0, "LARMOR", "FlipperCurrent");
  }

  void rfStateTest(const std::vector<double> &logMeans, const std::vector<char> &expectedSpinStates,
                   const std::string &instrumentName, const std::string &sfLogName) {
    auto wsGroup =
        createWorkpsaceGroupWithYValuesAndFlipperLogs({0.0, 0.0, 0.0, 0.0}, logMeans, instrumentName, sfLogName);

    DetermineSpinStateOrder alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", wsGroup);
    alg.execute();

    std::string result = alg.getPropertyValue("SpinStates");
    auto spinStates = Mantid::Kernel::StringTokenizer(result, ",").asVector();
    for (int i = 0; i < 4; ++i) {
      // get first char of the state string
      char rfState = spinStates[i][0];
      TS_ASSERT_EQUALS(rfState, expectedSpinStates[i]);
    }
  }

  void test_rfState_larmor() {
    const std::vector<double> logMeans = {6.0, 5.5, 0.0, 2.0};
    const std::vector<char> expectedSpinStates = {'-', '-', '+', '+'};
    rfStateTest(logMeans, expectedSpinStates, "LARMOR", "FlipperCurrent");
  }

  void test_rfState_zoom() {
    const std::vector<double> logMeans = {2.0, 3.5, -1.5, -4.0};
    const std::vector<char> expectedSpinStates = {'-', '-', '+', '+'};
    rfStateTest(logMeans, expectedSpinStates, "ZOOM", "Spin_flipper");
  }
};

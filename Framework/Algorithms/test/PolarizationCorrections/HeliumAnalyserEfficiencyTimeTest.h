// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiencyTime.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace HeAnalyserTimeTest {
static const std::string REFERENCE_NAME = "reference";
static const std::string INPUT_NAME = "input";
static const std::string GROUP_NAME = "group";
static const std::string OUTPUT_NAME = "out";
static const std::string TEST_ALGORITHM = "HeliumAnalyserEfficiencyTime";
static const std::string REF_TIMESTAMP = "2025-07-01T08:00:00";
constexpr double WAV_MIN = 1;
constexpr double WAV_MAX = 9;
constexpr double DEFAULT_LIFETIME = 45;
constexpr double DEFAULT_INI_POL = 0.9;
constexpr double DEFAULT_PXD = 12;
constexpr double DELTA = 1e-4;

template <typename T = MatrixWorkspace>
IAlgorithm_sptr prepareAlgorithm(const std::shared_ptr<T> &inputWorkspace, const std::string &refTimeStamp = "",
                                 const std::shared_ptr<T> &referenceWorkspace = nullptr) {
  const auto heAlgorithm = AlgorithmManager::Instance().create(TEST_ALGORITHM);
  heAlgorithm->initialize();
  heAlgorithm->setProperty("InputWorkspace", inputWorkspace);
  if (referenceWorkspace) {
    heAlgorithm->setProperty("ReferenceWorkspace", referenceWorkspace);
  }
  heAlgorithm->setProperty("ReferenceTimeStamp", refTimeStamp);
  heAlgorithm->setProperty("OutputWorkspace", OUTPUT_NAME);

  return heAlgorithm;
}

void algorithmExtraSettings(const IAlgorithm_sptr &algo, const double lifetime = DEFAULT_LIFETIME,
                            const double iniPol = DEFAULT_INI_POL, const double pxd = DEFAULT_PXD) {
  algo->setProperty("Lifetime", lifetime);
  algo->setProperty("PXD", pxd);
  algo->setProperty("InitialPolarization", iniPol);
}

MatrixWorkspace_sptr createWorkspace(const double binWidth = 1, const std::string &xUnit = "Wavelength",
                                     const std::string &refTimeStamp = REF_TIMESTAMP,
                                     const std::string &inputName = INPUT_NAME) {
  const auto createSampleWorkspace = AlgorithmManager::Instance().create("CreateSampleWorkspace");
  createSampleWorkspace->initialize();
  createSampleWorkspace->setProperty("WorkspaceType", "Histogram");
  createSampleWorkspace->setProperty("OutputWorkspace", inputName);
  createSampleWorkspace->setProperty("Function", "User Defined");
  createSampleWorkspace->setProperty("UserDefinedFunction", "name=LinearBackground,A0=1");
  createSampleWorkspace->setProperty("XUnit", xUnit);
  createSampleWorkspace->setProperty("XMin", WAV_MIN);
  createSampleWorkspace->setProperty("XMax", WAV_MAX);
  createSampleWorkspace->setProperty("BinWidth", binWidth);
  createSampleWorkspace->execute();

  const MatrixWorkspace_sptr result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputName);
  if (!refTimeStamp.empty()) {
    const auto start = Types::Core::DateAndTime(refTimeStamp);
    result->mutableRun().setStartAndEndTime(start, start + 1.0);
  }

  return result;
}

MatrixWorkspace_sptr getMatrixWorkspaceFromInput(const std::string &wsName) {
  const Workspace_sptr wksp = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
  MatrixWorkspace_sptr ws;
  if (wksp->isGroup()) {
    const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(wksp);
    ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  } else {
    ws = std::dynamic_pointer_cast<MatrixWorkspace>(wksp);
  }
  return ws;
}

std::vector<double> generateEfficiencyFunc(const double width = 1, const double factor = 1) {
  const auto size = static_cast<int>((WAV_MAX - WAV_MIN) / width);
  auto y = std::vector<double>(size);
  double x = WAV_MIN - 0.5 * width;
  for (auto index = 0; index < size; index++) {
    x += width;
    y.at(index) = 0.5 * (1 + std::tanh(factor * x));
  }
  return y;
}

double createFunctionArgument(const double lifetime = DEFAULT_LIFETIME, const double time = 1,
                              const double iniPol = DEFAULT_INI_POL, const double pxd = DEFAULT_PXD) {
  return 0.0733 * pxd * iniPol * std::exp(-time / lifetime);
}

// TimeDifference is a python algorithm. This is a basic mock for running the tests.
class TimeDifference final : public Algorithm {
public:
  const std::string name() const override { return "TimeDifference"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "TimeDifference Mock Algorithm"; }

private:
  void init() override {
    declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("InputWorkspaces"));
    declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "out",
                                                                         Mantid::Kernel::Direction::Output),
                    "");
    declareProperty(std::make_unique<WorkspaceProperty<>>("ReferenceWorkspace", "", Kernel::Direction::Input,
                                                          PropertyMode::Optional));
  };

  void exec() override {
    // Set error to 2 seconds (this will be a duration of 1 second).
    constexpr float sError = 2.0;
    constexpr float hError = sError / 3600;
    const ITableWorkspace_sptr outputTable = WorkspaceFactory::Instance().createTable();
    outputTable->addColumn("str", "ws_name");
    outputTable->addColumn("str", "midtime_stamp");
    outputTable->addColumn("float", "seconds");
    outputTable->addColumn("float", "seconds_error");
    outputTable->addColumn("float", "hours");
    outputTable->addColumn("float", "hours_error");

    const std::vector<std::string> workspaces = getProperty("InputWorkspaces");

    if (!isDefault("ReferenceWorkspace")) {
      TableRow newRow = outputTable->appendRow();
      constexpr float zero = 0;
      newRow << "ref" << REF_TIMESTAMP << zero << sError << zero << hError;
    }

    for (const auto &wsName : workspaces) {
      const auto ws = getMatrixWorkspaceFromInput(wsName);
      const auto timeStart = ws->mutableRun().getPropertyValueAsType<std::string>("start_time");
      const auto delay =
          static_cast<float>(DateAndTime::secondsFromDuration(DateAndTime(timeStart) - DateAndTime(REF_TIMESTAMP)));

      TableRow newRow = outputTable->appendRow();
      newRow << wsName << timeStart << delay << sError << delay / 3600 << hError;
    }
    setProperty("OutputWorkspace", outputTable);
  }
};

DECLARE_ALGORITHM(TimeDifference)

} // namespace HeAnalyserTimeTest

using namespace HeAnalyserTimeTest;
class HeliumAnalyserEfficiencyTimeTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_name() {
    HeliumAnalyserEfficiencyTime alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiencyTime");
  }

  void test_init() {
    HeliumAnalyserEfficiencyTime alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void test_algorithm_throws_for_non_wavelength_workspace() {
    const auto ws = createWorkspace(1, "TOF");
    const auto heAlgorithm = AlgorithmManager::Instance().create(TEST_ALGORITHM);
    heAlgorithm->initialize();

    TS_ASSERT_THROWS_EQUALS(heAlgorithm->setProperty("InputWorkspace", ws), std::invalid_argument const &e,
                            std::string(e.what()), "Workspace must have time logs and Wavelength units");
  }

  void test_algorithm_throws_for_workspace_without_time_logs() {
    const auto ws = createWorkspace(1, "Wavelength");
    auto &run = ws->mutableRun();
    run.removeProperty("start_time");
    run.removeProperty("run_start");
    run.removeProperty("end_time");
    run.removeProperty("run_end");

    const auto heAlgorithm = AlgorithmManager::Instance().create(TEST_ALGORITHM);
    heAlgorithm->initialize();

    TS_ASSERT_THROWS_EQUALS(heAlgorithm->setProperty("InputWorkspace", ws), std::invalid_argument const &e,
                            std::string(e.what()), "Workspace must have time logs and Wavelength units");
  }

  void test_algorithm_throws_when_no_timestamp_is_provided_in_any_way() {
    const auto ws = createWorkspace(1, "Wavelength");
    const auto alg = prepareAlgorithm(ws);

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n ReferenceWorkspace: Both ReferenceWorkspace and "
                            "ReferenceTimeStamp properties are empty, "
                            "at least one of the two has to be supplied to execute the Algorithm");
  }

  void test_algorithm_executes_for_default_timestamp() {
    auto ws = createWorkspace();
    const auto alg = prepareAlgorithm(ws, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
  }

  void test_algorithm_can_accept_groups_as_input() {
    const auto ws = createWorkspace();
    const auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(ws);
    AnalysisDataService::Instance().addOrReplace(GROUP_NAME, group);
    const auto alg = prepareAlgorithm(group, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
  }

  void test_algorithm_output_with_string_time_stamp_input() {
    const auto ws = createWorkspace(1, "Wavelength", "2025-07-01T09:00:00");
    const auto alg = prepareAlgorithm(ws, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateEfficiencyFunc(1, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }

  void test_algorithm_output_with_reference_workspace_input() {
    const auto ws = createWorkspace(1, "Wavelength", "2025-07-01T09:00:00");
    const auto ref = createWorkspace(1, "Wavelength", REF_TIMESTAMP, REFERENCE_NAME);
    const auto alg = prepareAlgorithm(ws, "", ref);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateEfficiencyFunc(1, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }

  void test_reference_workspace_takes_precedence_over_timestamp_if_both_are_provided() {
    const auto ws = createWorkspace(1, "Wavelength", "2025-07-01T09:00:00");
    const auto ref = createWorkspace(1, "Wavelength", REF_TIMESTAMP, REFERENCE_NAME);
    const auto alg = prepareAlgorithm(ws, "2011-11-11T11:11:11", ref);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateEfficiencyFunc(1, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }
};

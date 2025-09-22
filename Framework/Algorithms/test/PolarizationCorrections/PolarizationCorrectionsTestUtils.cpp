// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PolarizationCorrectionsTestUtils.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>

using namespace Mantid;
using namespace Mantid::API;

namespace PolCorrTestUtils {

std::string fillFuncStr(const std::vector<double> &number, const std::string &funcStr) {
  auto outStr(funcStr);
  for (const auto &num : number) {
    outStr.replace(outStr.find("#"), 1, std::to_string(num));
  }
  return outStr;
}

MatrixWorkspace_sptr generateFunctionDefinedWorkspace(const TestWorkspaceParameters &parameters,
                                                      const std::string &name, const std::string &func) {
  const auto &wsName = name.empty() ? parameters.testName : name;
  const auto &fitFunc = func.empty() ? parameters.funcStr : func;
  const auto createSampleWorkspace = AlgorithmManager::Instance().create("CreateSampleWorkspace");
  createSampleWorkspace->initialize();
  createSampleWorkspace->setProperty("WorkspaceType", "Histogram");
  createSampleWorkspace->setProperty("OutputWorkspace", wsName);
  createSampleWorkspace->setProperty("Function", "User Defined");
  createSampleWorkspace->setProperty("UserDefinedFunction", fitFunc);
  createSampleWorkspace->setProperty("XUnit", parameters.xUnit);
  createSampleWorkspace->setProperty("XMin", parameters.xMin);
  createSampleWorkspace->setProperty("XMax", parameters.xMax);
  createSampleWorkspace->setProperty("BinWidth", parameters.binWidth);
  createSampleWorkspace->setProperty("NumBanks", parameters.numBanks);
  createSampleWorkspace->setProperty("BankPixelWidth", 1);
  createSampleWorkspace->execute();

  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
  ws->setYUnit("Counts");
  ws->setDistribution(true);

  // Only needed for analyser eff tests. We are working with delays in hours but DateAndTime overloads + operator
  // with seconds for type double, thus factor 3600
  const auto timeOrigin = Types::Core::DateAndTime(parameters.refTimeStamp);
  const auto start = timeOrigin + 3600 * parameters.delay;
  ws->mutableRun().setStartAndEndTime(start, start + 1.0);

  return ws;
}

WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const std::vector<std::string> &funcs, const bool isFullPolarized) {
  // Polarized group with custom user functions in each member
  const auto &inputNames = isFullPolarized
                               ? std::vector({outName + "_11", outName + "_10", outName + "_01", outName + "_00"})
                               : std::vector({outName + "_00", outName + "_01"});
  for (size_t index = 0; index < inputNames.size(); index++) {
    generateFunctionDefinedWorkspace(parameters, inputNames.at(index), funcs.at(index));
  }
  return groupWorkspaces(outName, inputNames);
}

WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const std::vector<double> &amplitudes, const bool isFullPolarized) {
  // Polarized group with flat workspaces each of different amplitude
  const std::string defaultUserFunc = "name=UserFunction, Formula=x*0+";
  std::vector<std::string> funcs;
  std::transform(amplitudes.cbegin(), amplitudes.cend(), std::back_inserter(funcs),
                 [&defaultUserFunc](double amp) { return defaultUserFunc + std::to_string(amp); });
  return createPolarizedTestGroup(outName, parameters, funcs, isFullPolarized);
}

WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const bool isFullPolarized) {
  // Polarized group with same parameters for each member
  return createPolarizedTestGroup(outName, parameters, std::vector<std::string>(4), isFullPolarized);
}

WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<std::string> &wsToGroup) {
  const auto groupWorkspace = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWorkspace->initialize();
  groupWorkspace->setProperty("InputWorkspaces", wsToGroup);
  groupWorkspace->setProperty("OutputWorkspace", name);
  groupWorkspace->execute();
  WorkspaceGroup_sptr group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);
  return group;
}

WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup) {
  std::vector<std::string> wsToGroupNames(wsToGroup.size());
  std::transform(wsToGroup.cbegin(), wsToGroup.cend(), wsToGroupNames.begin(),
                 [](const MatrixWorkspace_sptr &w) { return w->getName(); });
  return groupWorkspaces(name, wsToGroupNames);
}

MatrixWorkspace_sptr getMatrixWorkspaceFromInput(const std::string &wsName) {
  Workspace_sptr wksp = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
  if (wksp->isGroup()) {
    const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(wksp);
    wksp = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  }
  return std::dynamic_pointer_cast<MatrixWorkspace>(wksp);
}

DECLARE_ALGORITHM(TimeDifference);

void TimeDifference::init() {
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("InputWorkspaces"));
  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "out", Mantid::Kernel::Direction::Output),
      "");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ReferenceWorkspace", "", Kernel::Direction::Input,
                                                        PropertyMode::Optional));
};

void TimeDifference::exec() {
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
} // namespace PolCorrTestUtils

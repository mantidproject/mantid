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
std::pair<std::vector<double>, std::vector<double>> createXYFromParams(double Xmin, double Xmax, double step,
                                                                       double Y) {
  const auto boundaries = static_cast<int>((Xmax - Xmin) / step);
  auto x = std::vector<double>(boundaries);
  auto y = std::vector<double>(boundaries);
  std::fill(y.begin(), y.end(), Y);
  std::generate(x.begin(), x.end(), [n = 0, &step, &Xmin]() mutable { return Xmin + step * n++; });
  return std::make_pair(x, y);
}

MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                       const std::vector<double> &y, const std::string &xUnit, const int nSpec,
                                       const double delay, const std::string &refTimeStamp) {
  const auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
  createWorkspace->initialize();
  createWorkspace->setProperty("DataX", x);
  createWorkspace->setProperty("DataY", y);
  createWorkspace->setProperty("UnitX", xUnit);
  createWorkspace->setProperty("NSpec", nSpec);
  createWorkspace->setProperty("OutputWorkspace", name);
  createWorkspace->execute();

  const auto convertToHistogram = AlgorithmManager::Instance().create("ConvertToHistogram");
  convertToHistogram->initialize();
  convertToHistogram->setProperty("InputWorkspace", name);
  convertToHistogram->setProperty("OutputWorkspace", name);
  convertToHistogram->execute();

  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
  // We are working with delays in hours but DateAndTime overloads + operator with seconds for type double, thus factor
  // 3600
  const auto timeOrigin = Types::Core::DateAndTime(refTimeStamp);
  const auto start = timeOrigin + 3600 * delay;
  ws->mutableRun().setStartAndEndTime(start, start + 1.0);
  return ws;
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

MatrixWorkspace_sptr generateFunctionDefinedWorkspace(const TestWorkspaceParameters &parameters) {
  const auto createSampleWorkspace = AlgorithmManager::Instance().create("CreateSampleWorkspace");
  createSampleWorkspace->initialize();
  createSampleWorkspace->setProperty("WorkspaceType", "Histogram");
  createSampleWorkspace->setProperty("OutputWorkspace", parameters.testName);
  createSampleWorkspace->setProperty("Function", "User Defined");
  createSampleWorkspace->setProperty("UserDefinedFunction", parameters.funcStr);
  createSampleWorkspace->setProperty("XUnit", parameters.xUnit);
  createSampleWorkspace->setProperty("XMin", parameters.xMin);
  createSampleWorkspace->setProperty("XMax", parameters.xMax);
  createSampleWorkspace->setProperty("BinWidth", parameters.binWidth);
  createSampleWorkspace->setProperty("NumBanks", parameters.numBanks);
  createSampleWorkspace->setProperty("BankPixelWidth", 1);
  createSampleWorkspace->execute();

  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(parameters.testName);
  ws->setYUnit("");
  ws->setDistribution(true);

  if (parameters.delay != 0) {
    // Only needed for analyser eff tests. We are working with delays in hours but DateAndTime overloads + operator
    // with seconds for type double, thus factor 3600
    const auto timeOrigin = Types::Core::DateAndTime(parameters.refTimeStamp);
    const auto start = timeOrigin + 3600 * parameters.delay;
    ws->mutableRun().setStartAndEndTime(start, start + 1.0);
  }
  return ws;
}

WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, TestWorkspaceParameters &parameters,
                                             const std::optional<std::vector<double>> &amplitudes,
                                             const bool isFullPolarized) {
  const auto &inputNames = isFullPolarized
                               ? std::vector({outName + "_11", outName + "_10", outName + "_01", outName + "_00"})
                               : std::vector({outName + "_00", outName + "_01"});
  const std::string defaultUserFunc = "name=UserFunction, Formula=x*0+";
  for (size_t index = 0; index < inputNames.size(); index++) {
    parameters.testName = inputNames.at(index);
    if (amplitudes.has_value()) {
      parameters.funcStr = defaultUserFunc + std::to_string(amplitudes->at(index));
    }
    generateFunctionDefinedWorkspace(parameters);
  }
  return groupWorkspaces(outName, inputNames);
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

IAlgorithm_sptr prepareHeEffAlgorithm(const std::vector<std::string> &inputWorkspaces, const std::string &outputName,
                                      const std::string &spinState, const std::string &outputFitParameters,
                                      const std::string &outputFitCurves) {
  const auto heAlgorithm = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
  heAlgorithm->initialize();
  heAlgorithm->setProperty("InputWorkspaces", inputWorkspaces);
  heAlgorithm->setProperty("SpinStates", spinState);
  heAlgorithm->setProperty("OutputWorkspace", outputName);

  heAlgorithm->setProperty("OutputFitParameters", outputFitParameters);
  heAlgorithm->setProperty("OutputFitCurves", outputFitCurves);
  return heAlgorithm;
}

std::vector<double> generateOutputFunc(const std::vector<double> &x, const double factor, const double mu,
                                       const bool efficiency) {

  auto y = std::vector<double>(x.size());

  for (size_t index = 0; index < x.size(); index++) {
    if (efficiency) {
      y.at(index) = 0.5 * (1 + std::tanh(factor * x.at(index)));
    } else {
      y.at(index) = std::exp(-mu * x.at(index)) * std::cosh(factor * x.at(index));
    }
  }
  return y;
}

double createFunctionArgument(const double lifetime, const double time, const double iniPol, const double pxd) {
  return LAMBDA_CONVERSION_FACTOR * pxd * iniPol * std::exp(-time / lifetime);
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

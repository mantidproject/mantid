// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionAlgorithmUtils.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

#include <filesystem>

namespace {
struct SumFilesData {
  std::string instrumentName;
  std::vector<std::string> wsNames;
  std::vector<int> runNo;

  explicit SumFilesData(const size_t vectorSize) {
    wsNames.reserve(vectorSize);
    runNo.reserve(vectorSize);
  }
};

void removeWorkspacesFromADS(const std::vector<std::string> &workspaceNames, bool deleteMonitors) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  for (size_t index = 0; index < workspaceNames.size(); index++) {
    const auto &wsName = workspaceNames[index];
    if (ads.doesExist(wsName)) {
      ads.remove(wsName);
    }
    if (const auto &monitorName = wsName + "_mon"; ads.doesExist(monitorName) && deleteMonitors) {
      ads.remove(monitorName);
    }
  }
}

Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string &name) {
  auto algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name);
  algorithm->initialize();
  algorithm->setRethrows(true);
  return algorithm;
}

std::string workspaceNameFromFilename(const std::string &filename) {
  return std::filesystem::path(filename).stem().string();
}

void loadFile(const std::string &filename, const std::string &ipfFilename, bool loadLogFiles, SumFilesData &sumData) {
  const auto workspaceName = workspaceNameFromFilename(filename);

  const auto loader = createAlgorithm("Load");
  loader->setPropertyValue("Filename", filename);
  loader->setPropertyValue("OutputWorkspace", workspaceName);

  if (loader->existsProperty("LoadLogFiles"))
    loader->setProperty("LoadLogFiles", loadLogFiles);

  loader->execute();

  const auto workspace =
      Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);

  const auto loadParameters = createAlgorithm("LoadParameterFile");
  loadParameters->setProperty("Workspace", workspaceName);
  loadParameters->setPropertyValue("Filename", ipfFilename);
  loadParameters->execute();

  const auto inst = workspace->getInstrument();

  if (sumData.instrumentName.empty()) {
    sumData.instrumentName = inst->getName();
  }
  sumData.wsNames.emplace_back(workspaceName);
  sumData.runNo.emplace_back(workspace->getRunNumber());
}

void mergeRuns(std::vector<std::string> const &inputNames, const std::string &outputName) {
  const auto mergeAlg = createAlgorithm("MergeRuns");
  mergeAlg->setProperty("InputWorkspaces", inputNames);
  mergeAlg->setPropertyValue("OutputWorkspace", outputName);
  mergeAlg->execute();
}

void scaleWorkspace(const std::string &workspaceName, double factor) {
  const auto scaleAlg = createAlgorithm("Scale");
  scaleAlg->setPropertyValue("InputWorkspace", workspaceName);
  scaleAlg->setPropertyValue("OutputWorkspace", workspaceName);
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->setPropertyValue("Operation", "Multiply");
  scaleAlg->execute();
}

void sumRegularRuns(std::vector<std::string> workspaceNames, bool isMonitor) {
  auto outputName = workspaceNames.front();
  const auto scaleFactor = 1.0 / static_cast<double>(workspaceNames.size());

  if (isMonitor) {
    outputName += "_mon";
    for (size_t i = 0; i < workspaceNames.size(); i++) {
      workspaceNames.at(i) += "_mon";
    }
  }
  mergeRuns(workspaceNames, outputName);
  scaleWorkspace(outputName, scaleFactor);
}

std::string renameOutputWorkspace(const SumFilesData &sumData) {
  const auto currName = sumData.wsNames.front();
  auto inst = sumData.instrumentName;
  if (inst == "TOSCA") {
    inst = "TSC";
  } else if (inst == "TFXA") {
    inst = "TFX";
  }

  std::string runstr = std::to_string(sumData.runNo.at(0));
  if (sumData.runNo.size() > 1) {
    const auto &[minRun, maxRun] = std::minmax_element(sumData.runNo.cbegin(), sumData.runNo.cend());
    runstr = std::to_string(*minRun) + "-" + std::to_string(*maxRun);
  }
  const auto wsName = inst + runstr + "_summed";
  const auto rename = createAlgorithm("RenameWorkspace");
  rename->setPropertyValue("InputWorkspace", currName);
  rename->setPropertyValue("OutputWorkspace", wsName);
  rename->execute();

  return wsName;
}

} // namespace

namespace MantidQt::CustomInterfaces {

using namespace Mantid::API;

MantidQt::API::IConfiguredAlgorithm_sptr configureAlgorithm(const std::string &algorithmName,
                                                            std::unique_ptr<IAlgorithmRuntimeProps> properties,
                                                            bool const validatePropsPreExec = true) {
  return std::make_unique<MantidQt::API::ConfiguredAlgorithm>(AlgorithmManager::Instance().create(algorithmName),
                                                              std::move(properties), validatePropsPreExec);
}

MantidQt::API::IConfiguredAlgorithm_sptr loadConfiguredAlg(std::string const &filename, std::string const &instrument,
                                                           std::vector<int> const &detectorList,
                                                           std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("Filename", filename, *properties);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  if (instrument == "TFXA") {
    AlgorithmProperties::update("LoadLogFiles", false, *properties);
    AlgorithmProperties::update("SpectrumMin", detectorList.front(), *properties);
    AlgorithmProperties::update("SpectrumMax", detectorList.back(), *properties);
  }
  return configureAlgorithm("Load", std::move(properties), false);
}

MantidQt::API::IConfiguredAlgorithm_sptr calculateFlatBackgroundConfiguredAlg(std::string const &inputWorkspace,
                                                                              double const startX, double const endX,
                                                                              std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("Mode", std::string("Mean"), *properties);
  AlgorithmProperties::update("StartX", startX, *properties);
  AlgorithmProperties::update("EndX", endX, *properties);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  return configureAlgorithm("CalculateFlatBackground", std::move(properties));
}

MantidQt::API::IConfiguredAlgorithm_sptr groupDetectorsConfiguredAlg(std::string const &inputWorkspace,
                                                                     std::vector<int> const &detectorList,
                                                                     std::string const &outputWorkspace) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("DetectorList", detectorList, *properties, false);
  AlgorithmProperties::update("OutputWorkspace", outputWorkspace, *properties);
  return configureAlgorithm("GroupDetectors", std::move(properties));
}

std::string loadFilesWithSum(const std::vector<std::string> &filenames, const std::string &ipfFilename,
                             bool loadLogFiles, bool deleteMonitors) {

  auto sumData = SumFilesData(filenames.size());
  for (const auto &filename : filenames) {
    loadFile(filename, ipfFilename, loadLogFiles, sumData);
  }

  if (filenames.size() == 1) {
    return filenames.at(0);
  }
  sumRegularRuns(sumData.wsNames, false);
  const auto outName = renameOutputWorkspace(sumData);
  removeWorkspacesFromADS(sumData.wsNames, deleteMonitors);

  return outName;
}

} // namespace MantidQt::CustomInterfaces

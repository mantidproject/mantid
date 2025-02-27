// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QuasiModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

#include <memory>

namespace {
auto &ads = Mantid::API::AnalysisDataService::Instance();
auto &algorithmManager = Mantid::API::AlgorithmManager::Instance();
auto &configService = Mantid::Kernel::ConfigService::Instance();
} // namespace

namespace MantidQt::CustomInterfaces {
using namespace Mantid::API;

QuasiModel::QuasiModel()
    : m_sampleWorkspace(), m_resolutionWorkspace(), m_outputResult(), m_outputProbability(), m_outputFitGroup() {}

void QuasiModel::setSample(std::string const &workspaceName) {
  if (!ads.doesExist(workspaceName)) {
    return;
  }
  if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_sampleWorkspace = workspace;
  }
}

void QuasiModel::setResolution(std::string const &workspaceName) {
  if (!ads.doesExist(workspaceName)) {
    return;
  }
  if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_resolutionWorkspace = workspace;
  }
}

void QuasiModel::setOutputResult(std::string const &workspaceName) {
  if (!ads.doesExist(workspaceName)) {
    return;
  }
  if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_outputResult = workspace;
  }
}

void QuasiModel::setOutputProbability(std::string const &workspaceName) {
  if (!ads.doesExist(workspaceName)) {
    return;
  }
  if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_outputProbability = workspace;
  }
}

void QuasiModel::setOutputFitGroup(std::string const &workspaceName) {
  if (!ads.doesExist(workspaceName)) {
    return;
  }
  if (auto const workspace = ads.retrieveWS<WorkspaceGroup>(workspaceName)) {
    m_outputFitGroup = workspace;
  }
}

MatrixWorkspace_sptr QuasiModel::outputFit(std::size_t const index) const {
  if (!m_outputFitGroup || static_cast<std::size_t>(m_outputFitGroup->getNumberOfEntries()) <= index) {
    return nullptr;
  }
  return std::dynamic_pointer_cast<MatrixWorkspace>(m_outputFitGroup->getItem(index));
}

bool QuasiModel::isResolution(std::string const &workspaceName) const {
  return Mantid::Kernel::Strings::endsWith(workspaceName, "_res");
}

std::optional<std::string> QuasiModel::curveColour(std::string const &label) const {
  if (label.find("fit 1") != std::string::npos) {
    return "red";
  } else if (label.find("fit 2") != std::string::npos) {
    return "magenta";
  } else if (label.find("diff 1") != std::string::npos) {
    return "blue";
  } else if (label.find("diff 2") != std::string::npos) {
    return "cyan";
  }
  return std::nullopt;
}

API::IConfiguredAlgorithm_sptr
QuasiModel::setupBayesQuasiAlgorithm(std::string const &resNormName, std::string const &fixWidthName,
                                     std::string const &program, std::string const &baseName,
                                     std::string const &background, double const eMin, double const eMax,
                                     int const sampleBinning, int const resolutionBinning, bool const elasticPeak,
                                     bool const fixWidth, bool const useResNorm, bool const sequentialFit) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  properties->setProperty("Program", program);
  properties->setProperty("SampleWorkspace", m_sampleWorkspace->getName());
  properties->setProperty("ResolutionWorkspace", m_resolutionWorkspace->getName());
  properties->setProperty("OutputWorkspaceFit", baseName + "_Fit");
  properties->setProperty("OutputWorkspaceProb", baseName + "_Prob");
  properties->setProperty("OutputWorkspaceResult", baseName + "_Result");
  properties->setProperty("Elastic", elasticPeak);
  properties->setProperty("ResNormWorkspace", resNormName);
  properties->setProperty("Background", background);
  properties->setProperty("MinRange", eMin);
  properties->setProperty("MaxRange", eMax);
  properties->setProperty("SampleBins", sampleBinning);
  properties->setProperty("ResolutionBins", resolutionBinning);
  properties->setProperty("FixedWidth", fixWidth);
  properties->setProperty("UseResNorm", useResNorm);
  properties->setProperty("WidthFile", fixWidthName);
  properties->setProperty("Loop", sequentialFit);

  auto quasiAlgorithm = algorithmManager.create("BayesQuasi");
  quasiAlgorithm->initialize();

  return std::make_shared<API::ConfiguredAlgorithm>(std::move(quasiAlgorithm), std::move(properties));
}

API::IConfiguredAlgorithm_sptr QuasiModel::setupBayesQuasi2Algorithm(std::string const &program,
                                                                     std::string const &baseName,
                                                                     std::string const &background, double const eMin,
                                                                     double const eMax, bool const elasticPeak) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  properties->setProperty("Program", program);
  properties->setProperty("SampleWorkspace", m_sampleWorkspace->getName());
  properties->setProperty("ResolutionWorkspace", m_resolutionWorkspace->getName());
  properties->setProperty("OutputWorkspaceFit", baseName + "_Fit");
  properties->setProperty("OutputWorkspaceProb", baseName + "_Prob");
  properties->setProperty("OutputWorkspaceResult", baseName + "_Result");
  properties->setProperty("Elastic", elasticPeak);
  properties->setProperty("Background", background == "Flat" ? background : "None");
  properties->setProperty("EMin", eMin);
  properties->setProperty("EMax", eMax);

  auto quasiAlgorithm = algorithmManager.create("BayesQuasi2");
  quasiAlgorithm->initialize();

  return std::make_shared<API::ConfiguredAlgorithm>(std::move(quasiAlgorithm), std::move(properties));
}

API::IConfiguredAlgorithm_sptr QuasiModel::setupSaveAlgorithm(Mantid::API::Workspace_sptr workspace) const {
  auto const saveDirectory = configService.getString("defaultsave.directory");

  auto saveAlgorithm = algorithmManager.create("SaveNexusProcessed");
  saveAlgorithm->initialize();

  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  properties->setProperty("Filename", saveDirectory + workspace->getName() + ".nxs");
  properties->setProperty("InputWorkspace", workspace);

  return std::make_shared<API::ConfiguredAlgorithm>(std::move(saveAlgorithm), std::move(properties));
}

} // namespace MantidQt::CustomInterfaces

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ElwinModel.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt::CustomInterfaces {

ElwinModel::ElwinModel()
    : m_integrationStart(), m_integrationEnd(), m_backgroundStart(), m_backgroundEnd(), m_backgroundSubtraction(),
      m_normalise(), m_outputWorkspaceNames() {}

void ElwinModel::setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &filepath,
                                    std::string const &outputName) {
  auto loadAlg = AlgorithmManager::Instance().create("LoadNexus");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filepath);
  loadAlg->setProperty("OutputWorkspace", outputName);
  batchAlgoRunner->addAlgorithm(loadAlg);
}

void ElwinModel::setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                     std::string const &inputWorkspacesString, std::string const &inputGroupWsName) {
  auto groupWsAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWsAlg->initialize();
  auto runtimeProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  runtimeProps->setPropertyValue("InputWorkspaces", inputWorkspacesString);
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);
  batchAlgoRunner->addAlgorithm(groupWsAlg, std::move(runtimeProps));
}

void ElwinModel::setupElasticWindowMultiple(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                            std::string const &workspaceBaseName, std::string const &inputGroupWsName,
                                            std::string const &sampleEnvironmentLogName,
                                            std::string const &sampleEnvironmentLogValue) {

  setOutputWorkspaceNames(workspaceBaseName);

  // Configure ElasticWindowMultiple algorithm
  auto elwinMultAlg = AlgorithmManager::Instance().create("ElasticWindowMultiple");
  elwinMultAlg->initialize();

  elwinMultAlg->setProperty("OutputInQ", m_outputWorkspaceNames["qWorkspace"]);
  elwinMultAlg->setProperty("OutputInQSquared", m_outputWorkspaceNames["qSquaredWorkspace"]);
  elwinMultAlg->setProperty("OutputELF", m_outputWorkspaceNames["elfWorkspace"]);

  elwinMultAlg->setProperty("SampleEnvironmentLogName", sampleEnvironmentLogName);
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", sampleEnvironmentLogValue);

  elwinMultAlg->setProperty("IntegrationRangeStart", m_integrationStart);
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_integrationEnd);

  if (m_backgroundSubtraction) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_backgroundStart);
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_backgroundEnd);
  }

  if (m_normalise) {
    elwinMultAlg->setProperty("OutputELT", m_outputWorkspaceNames["eltWorkspace"]);
  }
  auto runtimeProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  runtimeProps->setPropertyValue("InputWorkspaces", inputGroupWsName);
  batchAlgoRunner->addAlgorithm(elwinMultAlg, std::move(runtimeProps));
}

void ElwinModel::ungroupAlgorithm(std::string const &inputWorkspace) const {
  auto ungroupAlg = AlgorithmManager::Instance().create("UnGroupWorkspace");
  ungroupAlg->initialize();
  ungroupAlg->setProperty("InputWorkspace", inputWorkspace);
  ungroupAlg->execute();
}

void ElwinModel::groupAlgorithm(std::string const &inputWorkspaces, std::string const &outputWorkspace) const {
  auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setProperty("InputWorkspaces", inputWorkspaces);
  groupAlg->setProperty("OutputWorkspace", outputWorkspace);
  groupAlg->execute();
}

std::string ElwinModel::createGroupedWorkspaces(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra) {
  auto extractSpectra = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extractSpectra->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  extractSpectra->setProperty("OutputWorkspace", workspace->getName() + "_extracted_spectra");
  extractSpectra->setProperty("WorkspaceIndex", std::to_string(spectra[0].value));
  extractSpectra->execute();

  for (size_t j = 1; j < spectra.size().value; j++) {
    extractSpectra->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
    extractSpectra->setProperty("OutputWorkspace", "specWSnext");
    extractSpectra->setProperty("WorkspaceIndex", std::to_string(spectra[j].value));
    extractSpectra->execute();
    auto appendSpectra = AlgorithmManager::Instance().create("AppendSpectra");
    appendSpectra->setProperty("InputWorkspace1", workspace->getName() + "_extracted_spectra");
    appendSpectra->setProperty("InputWorkspace2", "specWSnext");
    appendSpectra->setProperty("AppendYAxisLabels", true);
    appendSpectra->setProperty("OutputWorkspace", workspace->getName() + "_extracted_spectra");
    appendSpectra->execute();
  }
  AnalysisDataService::Instance().remove("specWSnext");
  return workspace->getName() + "_extracted_spectra";
}
void ElwinModel::setOutputWorkspaceNames(std::string const &workspaceBaseName) {
  auto elwinSuffix = "_elwin_";
  m_outputWorkspaceNames["qWorkspace"] = workspaceBaseName + elwinSuffix + "eq";
  m_outputWorkspaceNames["qSquaredWorkspace"] = workspaceBaseName + elwinSuffix + "eq2";
  m_outputWorkspaceNames["elfWorkspace"] = workspaceBaseName + elwinSuffix + "elf";
  m_outputWorkspaceNames["eltWorkspace"] = workspaceBaseName + elwinSuffix + "elt";
}

std::string ElwinModel::getOutputWorkspaceNames() const {
  std::vector<std::string> keys = {"qWorkspace", "qSquaredWorkspace", "elfWorkspace", "eltWorkspace"};
  std::ostringstream oss;
  std::transform(keys.cbegin(), keys.cend(), std::ostream_iterator<std::string>(oss, ","),
                 [&](const auto &key) { return m_outputWorkspaceNames.at(key); });
  std::string outputWorkspaceNames = oss.str();
  outputWorkspaceNames.resize(outputWorkspaceNames.size() - 1);
  return outputWorkspaceNames;
}

void ElwinModel::setIntegrationStart(double integrationStart) { m_integrationStart = integrationStart; }

void ElwinModel::setIntegrationEnd(double integrationEnd) { m_integrationEnd = integrationEnd; }

void ElwinModel::setBackgroundStart(double backgroundStart) { m_backgroundStart = backgroundStart; }

void ElwinModel::setBackgroundEnd(double backgroundEnd) { m_backgroundEnd = backgroundEnd; }

void ElwinModel::setBackgroundSubtraction(bool backgroundSubtraction) {
  m_backgroundSubtraction = backgroundSubtraction;
}

void ElwinModel::setNormalise(bool normalise) { m_normalise = normalise; }

} // namespace MantidQt::CustomInterfaces

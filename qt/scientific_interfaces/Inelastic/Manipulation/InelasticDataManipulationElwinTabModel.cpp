// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTabModel.h"
#include "Common/IndirectDataValidationHelper.h"
#include "Common/WorkspaceUtils.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>
#include <regex>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
auto const regDigits = std::regex("\\d+");
}
namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InelasticDataManipulationElwinTabModel::InelasticDataManipulationElwinTabModel() {}

void InelasticDataManipulationElwinTabModel::setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                                                std::string const &filepath,
                                                                std::string const &outputName) {
  auto loadAlg = AlgorithmManager::Instance().create("LoadNexus");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filepath);
  loadAlg->setProperty("OutputWorkspace", outputName);
  batchAlgoRunner->addAlgorithm(loadAlg);
}

void InelasticDataManipulationElwinTabModel::setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                                                 std::string const &inputWorkspacesString,
                                                                 std::string const &inputGroupWsName) {
  auto groupWsAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWsAlg->initialize();
  auto runtimeProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  runtimeProps->setPropertyValue("InputWorkspaces", inputWorkspacesString);
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);
  batchAlgoRunner->addAlgorithm(groupWsAlg, std::move(runtimeProps));
}

void InelasticDataManipulationElwinTabModel::setupElasticWindowMultiple(
    MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &workspaceBaseName,
    std::string const &inputGroupWsName, std::string const &sampleEnvironmentLogName,
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

void InelasticDataManipulationElwinTabModel::ungroupAlgorithm(std::string const &inputWorkspace) const {
  auto ungroupAlg = AlgorithmManager::Instance().create("UnGroupWorkspace");
  ungroupAlg->initialize();
  ungroupAlg->setProperty("InputWorkspace", inputWorkspace);
  ungroupAlg->execute();
}

void InelasticDataManipulationElwinTabModel::groupAlgorithm(std::string const &inputWorkspaces,
                                                            std::string const &outputWorkspace) const {
  auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setProperty("InputWorkspaces", inputWorkspaces);
  groupAlg->setProperty("OutputWorkspace", outputWorkspace);
  groupAlg->execute();
}

std::string InelasticDataManipulationElwinTabModel::createGroupedWorkspaces(MatrixWorkspace_sptr workspace,
                                                                            FunctionModelSpectra const &spectra) {
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
void InelasticDataManipulationElwinTabModel::setOutputWorkspaceNames(std::string const &workspaceBaseName) {
  auto elwinSuffix = "_elwin_";
  m_outputWorkspaceNames["qWorkspace"] = workspaceBaseName + elwinSuffix + "eq";
  m_outputWorkspaceNames["qSquaredWorkspace"] = workspaceBaseName + elwinSuffix + "eq2";
  m_outputWorkspaceNames["elfWorkspace"] = workspaceBaseName + elwinSuffix + "elf";
  m_outputWorkspaceNames["eltWorkspace"] = workspaceBaseName + elwinSuffix + "elt";
}

std::string InelasticDataManipulationElwinTabModel::getOutputWorkspaceNames() const {
  std::ostringstream oss;
  std::transform(m_outputWorkspaceNames.cbegin(), m_outputWorkspaceNames.cend(),
                 std::ostream_iterator<std::string>(oss, ","),
                 [](const auto &element) { return static_cast<std::string>(element.second); });
  std::string outputWorkspaceNames = oss.str();
  outputWorkspaceNames.resize(outputWorkspaceNames.size() - 1);
  return outputWorkspaceNames;
}

std::string
InelasticDataManipulationElwinTabModel::prepareOutputPrefix(std::vector<std::string> const &workspaceNames) const {
  auto names = WorkspaceUtils::transformElements(workspaceNames.begin(), workspaceNames.end(),
                                                 [](auto &name) { return name.substr(0, name.find_first_of('_')); });
  std::smatch match;
  std::vector<int> runNumbers;
  std::string prefix;
  std::string suffix = workspaceNames[0].substr(workspaceNames[0].find_first_of('_'));
  for (auto const &name : names)
    if (std::regex_search(name, match, regDigits)) {
      runNumbers.push_back(std::stoi(match.str(0)));
      if (prefix.empty())
        prefix = match.prefix().str();
    }
  if (runNumbers.empty() || runNumbers.size() == 1)
    return workspaceNames[0];
  else {
    auto [min, max] = std::minmax_element(runNumbers.cbegin(), runNumbers.cend());
    return prefix + std::to_string(*min) + "-" + std::to_string(*max) + suffix;
  }
}

void InelasticDataManipulationElwinTabModel::setIntegrationStart(double integrationStart) {
  m_integrationStart = integrationStart;
}

void InelasticDataManipulationElwinTabModel::setIntegrationEnd(double integrationEnd) {
  m_integrationEnd = integrationEnd;
}

void InelasticDataManipulationElwinTabModel::setBackgroundStart(double backgroundStart) {
  m_backgroundStart = backgroundStart;
}

void InelasticDataManipulationElwinTabModel::setBackgroundEnd(double backgroundEnd) { m_backgroundEnd = backgroundEnd; }

void InelasticDataManipulationElwinTabModel::setBackgroundSubtraction(bool backgroundSubtraction) {
  m_backgroundSubtraction = backgroundSubtraction;
}

void InelasticDataManipulationElwinTabModel::setNormalise(bool normalise) { m_normalise = normalise; }

} // namespace MantidQt::CustomInterfaces

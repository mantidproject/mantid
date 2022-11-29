// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTabModel.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

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
  auto runtimeProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  runtimeProps->setPropertyValue("InputWorkspaces", inputWorkspacesString);
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);
  batchAlgoRunner->addAlgorithm(groupWsAlg, std::move(runtimeProps));
}

void InelasticDataManipulationElwinTabModel::setupElasticWindowMultiple(
    MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, QString workspaceBaseName,
    std::string const &inputGroupWsName, std::string const &sampleEnvironmentLogName,
    std::string sampleEnvironmentLogValue) {

  workspaceBaseName += "_elwin_";

  const auto qWorkspace = (workspaceBaseName + "eq").toStdString();
  const auto qSquaredWorkspace = (workspaceBaseName + "eq2").toStdString();
  const auto elfWorkspace = (workspaceBaseName + "elf").toStdString();
  const auto eltWorkspace = (workspaceBaseName + "elt").toStdString();

  // Configure ElasticWindowMultiple algorithm
  auto elwinMultAlg = AlgorithmManager::Instance().create("ElasticWindowMultiple");
  elwinMultAlg->initialize();

  elwinMultAlg->setProperty("OutputInQ", qWorkspace);
  elwinMultAlg->setProperty("OutputInQSquared", qSquaredWorkspace);
  elwinMultAlg->setProperty("OutputELF", elfWorkspace);

  elwinMultAlg->setProperty("SampleEnvironmentLogName", sampleEnvironmentLogName);
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", sampleEnvironmentLogValue);

  elwinMultAlg->setProperty("IntegrationRangeStart", m_integrationStart);
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_integrationEnd);

  if (m_backgroundSubtraction) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_backgroundStart);
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_backgroundEnd);
  }

  if (m_normalise) {
    elwinMultAlg->setProperty("OutputELT", eltWorkspace);
  }
  auto runtimeProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  runtimeProps->setPropertyValue("InputWorkspaces", inputGroupWsName);
  batchAlgoRunner->addAlgorithm(elwinMultAlg, std::move(runtimeProps));
}

void InelasticDataManipulationElwinTabModel::ungroupAlgorithm(const std::string &InputWorkspace) {
  auto ungroupAlg = AlgorithmManager::Instance().create("UnGroupWorkspace");
  ungroupAlg->initialize();
  ungroupAlg->setProperty("InputWorkspace", InputWorkspace);
  ungroupAlg->execute();
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

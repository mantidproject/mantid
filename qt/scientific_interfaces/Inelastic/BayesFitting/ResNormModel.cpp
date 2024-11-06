// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ResNormModel.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>
#include <MantidQtWidgets/Common/WorkspaceUtils.h>
#include <map>
#include <string>

namespace MantidQt::CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

ResNormModel::ResNormModel() : m_eMin(0), m_eMax(0) {
  m_logs.setLogNames(StringVec(
      {{"sample_filename"}, {"resolution_filename"}, {"fit_program"}, {"create_output"}, {"e_min"}, {"e_max"}}));
  m_logs.setLogTypes(StringVec({{"String"}, {"String"}, {"String"}, {"String"}, {"Number"}, {"Number"}}));
}

double ResNormModel::eMin() const { return m_eMin; }
double ResNormModel::eMax() const { return m_eMax; }

void ResNormModel::setEMin(double const value) { m_eMin = value; }
void ResNormModel::setEMax(double const value) { m_eMax = value; }

API::IConfiguredAlgorithm_sptr ResNormModel::setupResNormAlgorithm(std::string const &outputWsName,
                                                                   std::string const &vanWorkspace,
                                                                   std::string const &resWorkspace) const {
  auto resNormAlgorithm = AlgorithmManager::Instance().create("ResNorm", 2);
  resNormAlgorithm->initialize();
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  properties->setProperty("VanadiumWorkspace", vanWorkspace);
  properties->setProperty("ResolutionWorkspace", resWorkspace);
  properties->setProperty("EnergyMin", m_eMin);
  properties->setProperty("EnergyMax", m_eMax);
  properties->setProperty("CreateOutput", true);
  properties->setProperty("OutputWorkspace", outputWsName);
  properties->setProperty("OutputWorkspaceTable", (outputWsName + "_Fit"));
  MantidQt::API::IConfiguredAlgorithm_sptr resNormConfAlgo =
      std::make_shared<API::ConfiguredAlgorithm>(resNormAlgorithm, std::move(properties));
  return resNormConfAlgo;
}

void ResNormModel::copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const Workspace_sptr &workspace) const {
  auto logCopier = AlgorithmManager::Instance().create("CopyLogs");
  logCopier->setProperty("InputWorkspace", resultWorkspace->getName());
  logCopier->setProperty("OutputWorkspace", workspace->getName());
  logCopier->execute();
}

void ResNormModel::addAdditionalLogs(const Workspace_sptr &resultWorkspace) const {
  auto logAdder = AlgorithmManager::Instance().create("AddSampleLogMultiple");
  auto const name = resultWorkspace->getName();
  logAdder->setProperty("Workspace", name);
  logAdder->setProperty("ParseType", false);
  logAdder->setProperty("LogNames", m_logs.logNames());
  logAdder->setProperty("LogTypes", m_logs.logTypes());
  logAdder->setProperty("LogValues", m_logs.logValues());
  logAdder->execute();
}

void ResNormModel::processLogs(std::string const &vanWsName, std::string const &resWsName,
                               std::string const &outputWsName) {
  updateLogs(vanWsName, resWsName);
  auto const resolutionWorkspace = getADSWorkspace(resWsName);
  auto const resultWorkspace = getADSWorkspace<WorkspaceGroup>(outputWsName);

  for (auto const &workspace : *resultWorkspace) {
    copyLogs(resolutionWorkspace, workspace);
    addAdditionalLogs(workspace);
  }
}

void ResNormModel::updateLogs(std::string const &vanadiumSampName, std::string const &resSampName) {
  m_logs.setLogValues(StringVec({{vanadiumSampName},
                                 {resSampName},
                                 {"ResNormPresenter"},
                                 {"true"},
                                 {boost::lexical_cast<std::string>(eMin())},
                                 {boost::lexical_cast<std::string>(eMax())}}));
}
API::IConfiguredAlgorithm_sptr ResNormModel::setupSaveAlgorithm(const std::string &wsName,
                                                                const std::string &filename) const {
  // Setup the algorithm
  auto saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlgo->initialize();
  if (filename.empty())
    saveAlgo->setProperty("Filename", wsName + ".nxs");
  else
    saveAlgo->setProperty("Filename", filename);
  // Setup the input workspace property
  auto saveProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  saveProps->setPropertyValue("InputWorkspace", wsName);

  MantidQt::API::IConfiguredAlgorithm_sptr saveConfAlgo =
      std::make_shared<API::ConfiguredAlgorithm>(saveAlgo, std::move(saveProps));
  return saveConfAlgo;
}
} // namespace MantidQt::CustomInterfaces

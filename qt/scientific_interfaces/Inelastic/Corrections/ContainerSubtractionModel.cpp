// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ContainerSubtractionModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <MantidAPI/AlgorithmRuntimeProps.h>

#include <MantidAPI/AnalysisDataService.h>
#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>

using namespace Mantid::API;

namespace ContainerSubtractionModelHelperAlgorithms {

static const std::string modPrefix = "__ContainerTransf";

MatrixWorkspace_sptr shiftWorkspace(const MatrixWorkspace_sptr &workspace, double shiftValue) {
  const IAlgorithm_sptr shift = AlgorithmManager::Instance().create("ScaleX");
  shift->initialize();
  shift->setChild(true);
  shift->setLogging(false);
  shift->setProperty("InputWorkspace", workspace);
  shift->setProperty("Operation", "Add");
  shift->setProperty("Factor", shiftValue);
  shift->setProperty("OutputWorkspace", "shifted");
  shift->execute();
  return shift->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr scaleWorkspace(const MatrixWorkspace_sptr &workspace, double scaleValue) {
  const IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setChild(true);
  scale->setLogging(false);
  scale->setProperty("InputWorkspace", workspace);
  scale->setProperty("Operation", "Multiply");
  scale->setProperty("Factor", scaleValue);
  scale->setProperty("OutputWorkspace", "scaled");
  scale->execute();
  return scale->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr convertToHistogram(const MatrixWorkspace_sptr &workspace) {
  const IAlgorithm_sptr convert = AlgorithmManager::Instance().create("ConvertToHistogram");
  convert->initialize();
  convert->setChild(true);
  convert->setLogging(false);
  convert->setProperty("InputWorkspace", workspace);
  convert->setProperty("OutputWorkspace", "converted");
  convert->execute();
  return convert->getProperty("OutputWorkspace");
}

} // namespace ContainerSubtractionModelHelperAlgorithms

namespace MantidQt::CustomInterfaces {
using namespace ContainerSubtractionModelHelperAlgorithms;

ContainerSubtractionModel::ContainerSubtractionModel()
    : m_csSampleWS(), m_csContainerWS(), m_csSubtractedWS(), m_csModContainerWS() {}

ContainerSubtractionModel::~ContainerSubtractionModel() {
  if (AnalysisDataService::Instance().doesExist(modPrefix)) {
    AnalysisDataService::Instance().remove(modPrefix);
  }
}

MatrixWorkspace_sptr ContainerSubtractionModel::rebinToWorkspace(const MatrixWorkspace_sptr &workspaceToRebin,
                                                                 const MatrixWorkspace_sptr &workspaceToMatch) const {
  const IAlgorithm_sptr rebin = AlgorithmManager::Instance().create("RebinToWorkspace");
  rebin->initialize();
  rebin->setChild(true);
  rebin->setLogging(false);
  rebin->setProperty("WorkspaceToRebin", workspaceToRebin);
  rebin->setProperty("WorkspaceToMatch", workspaceToMatch);
  rebin->setProperty("OutputWorkspace", "rebinned");
  rebin->execute();
  return rebin->getProperty("OutputWorkspace");
}

void ContainerSubtractionModel::addShiftLog(double shiftX) {
  IAlgorithm_sptr shiftLog = AlgorithmManager::Instance().create("AddSampleLog");
  shiftLog->initialize();
  shiftLog->setLogging(true);
  shiftLog->setProperty("Workspace", subtractedWS()->getName());
  shiftLog->setProperty("LogName", "container_shift");
  shiftLog->setProperty("LogType", "Number");
  shiftLog->setProperty("LogText", std::to_string(shiftX));
  shiftLog->execute();
}

API::IConfiguredAlgorithm_sptr
ContainerSubtractionModel::minusWorkspace(const MatrixWorkspace_sptr &lhsWorkspace,
                                          const MatrixWorkspace_sptr &rhsWorkspace) const {
  IAlgorithm_sptr minus = AlgorithmManager::Instance().create("Minus");
  minus->initialize();
  minus->setChild(true);
  minus->setLogging(false);
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  properties->setProperty("LHSWorkspace", lhsWorkspace);
  properties->setProperty("RHSWorkspace", rhsWorkspace);
  properties->setProperty("OutputWorkspace", "subtracted");
  auto confAlg = std::make_shared<API::ConfiguredAlgorithm>(minus, std::move(properties));
  return confAlg;
}

MatrixWorkspace_sptr ContainerSubtractionModel::newWorkspace(const std::string &wsName) const {
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName)) {
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    if (!ws->isHistogramData()) {
      ws = convertToHistogram(ws);
    }
  }
  return ws;
}

void ContainerSubtractionModel::updateContainer(double shiftX, double scale) {
  if (!canWS() || !sampleWS()) {
    return;
  }
  MatrixWorkspace_sptr updatedCanWs = canWS()->clone();
  if (shiftX != 0.0) {
    updatedCanWs = shiftWorkspace(updatedCanWs, shiftX);
    updatedCanWs = rebinToWorkspace(updatedCanWs, sampleWS());
  }
  if (scale != 1.0) {
    updatedCanWs = scaleWorkspace(updatedCanWs, scale);
  }
  auto &ads = AnalysisDataService::Instance();
  ads.addOrReplace(modPrefix, updatedCanWs);
  m_csModContainerWS = ads.retrieveWS<MatrixWorkspace>(modPrefix);
}

API::IConfiguredAlgorithm_sptr ContainerSubtractionModel::prepareSubtraction(double shiftX, double scale,
                                                                             bool doRebin) {
  if (!modCanWS()) {
    updateContainer(shiftX, scale);
  }
  if ((shiftX == 0.0) && doRebin) {
    m_csModContainerWS = rebinToWorkspace(modCanWS(), sampleWS());
  }
  return minusWorkspace(sampleWS(), modCanWS());
}

void ContainerSubtractionModel::setSampleWS(const std::string &name) { m_csSampleWS = newWorkspace(name); }
void ContainerSubtractionModel::setCanWS(const std::string &name) { m_csContainerWS = newWorkspace(name); }
void ContainerSubtractionModel::setSubtractedWS(const std::string &name) { m_csSubtractedWS = newWorkspace(name); }
void ContainerSubtractionModel::removeSubtractedWS() {
  m_csSubtractedWS.reset();
  m_csModContainerWS.reset();
}

const MatrixWorkspace_sptr &ContainerSubtractionModel::sampleWS() const { return m_csSampleWS; }
const MatrixWorkspace_sptr &ContainerSubtractionModel::canWS() const { return m_csContainerWS; }
const MatrixWorkspace_sptr &ContainerSubtractionModel::subtractedWS() const { return m_csSubtractedWS; }
const MatrixWorkspace_sptr &ContainerSubtractionModel::modCanWS() const { return m_csModContainerWS; }

std::vector<std::string> ContainerSubtractionModel::getAllValidWorkspaceNames() const {
  const auto workspaces = {sampleWS(), modCanWS() ? modCanWS() : canWS(), subtractedWS()};
  std::vector<std::string> names;
  for (const auto &ws : workspaces) {
    if (ws) {
      names.push_back(ws->getName());
    }
  }
  return names;
}

} // namespace MantidQt::CustomInterfaces

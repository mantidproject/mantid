// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SymmetriseModel.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

SymmetriseModel::SymmetriseModel()
    : m_inputWorkspace(), m_reflectedInputWorkspace(), m_negativeOutputWorkspace(), m_positiveOutputWorkspace(),
      m_eMin(), m_eMax(), m_isPositiveReflect(), m_spectraRange() {}

API::IConfiguredAlgorithm_sptr SymmetriseModel::setupPreviewAlgorithm(std::vector<int> const &spectraRange) {

  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
  }
  // Run the algorithm on the preview spectrum only, these outputs are only for plotting in the preview window and are
  // not accessed by users directly.
  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  symmetriseAlg->setRethrows(true);
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  properties->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflectedInputWorkspace);
  properties->setProperty("XMin", m_eMin);
  properties->setProperty("XMax", m_eMax);
  properties->setProperty("SpectraRange", spectraRange);
  properties->setProperty("OutputWorkspace", "__Symmetrise_temp");
  properties->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");
  auto confAlg = std::make_shared<API::ConfiguredAlgorithm>(symmetriseAlg, std::move(properties));
  return confAlg;
}

API::IConfiguredAlgorithm_sptr SymmetriseModel::setupSymmetriseAlgorithm() {

  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
  }

  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  properties->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflectedInputWorkspace);
  properties->setProperty("XMin", m_eMin);
  properties->setProperty("XMax", m_eMax);
  properties->setProperty("OutputWorkspace",
                          m_isPositiveReflect ? m_positiveOutputWorkspace : m_negativeOutputWorkspace);
  properties->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");
  auto confAlg = std::make_shared<API::ConfiguredAlgorithm>(symmetriseAlg, std::move(properties));
  return confAlg;
}

void SymmetriseModel::reflectNegativeToPositive() {
  IAlgorithm_sptr scaleXAlg = AlgorithmManager::Instance().create("ScaleX");
  scaleXAlg->initialize();
  scaleXAlg->setProperty("InputWorkspace", m_inputWorkspace);
  scaleXAlg->setProperty("Operation", "Multiply");
  scaleXAlg->setProperty("Factor", -1.0);
  scaleXAlg->setProperty("OutputWorkspace", m_reflectedInputWorkspace);
  scaleXAlg->execute();

  IAlgorithm_sptr sortXAxisAlg = AlgorithmManager::Instance().create("SortXAxis");
  sortXAxisAlg->initialize();
  sortXAxisAlg->setProperty("InputWorkspace", m_reflectedInputWorkspace);
  sortXAxisAlg->setProperty("OutputWorkspace", m_reflectedInputWorkspace);
  sortXAxisAlg->execute();
}

void SymmetriseModel::setWorkspaceName(std::string const &workspaceName) {
  m_inputWorkspace = workspaceName;
  m_reflectedInputWorkspace = m_inputWorkspace + "_reflected";
  // the last 4 characters in the workspace name are '_red' the ouput workspace name is inserting '_sym' before that
  // '_red'
  m_positiveOutputWorkspace = workspaceName.substr(0, workspaceName.size() - 4) + "_sym_pn" +
                              workspaceName.substr(workspaceName.size() - 4, workspaceName.size());
  m_negativeOutputWorkspace = workspaceName.substr(0, workspaceName.size() - 4) + "_sym_np" +
                              workspaceName.substr(workspaceName.size() - 4, workspaceName.size());
}

void SymmetriseModel::setEMin(double value) { m_eMin = value; }

void SymmetriseModel::setEMax(double value) { m_eMax = value; }

void SymmetriseModel::setIsPositiveReflect(bool value) { m_isPositiveReflect = value; }

bool SymmetriseModel::getIsPositiveReflect() const { return m_isPositiveReflect; }

} // namespace MantidQt::CustomInterfaces

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SymmetriseModel.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

SymmetriseModel::SymmetriseModel()
    : m_inputWorkspace(), m_reflectedInputWorkspace(), m_negativeOutputWorkspace(), m_positiveOutputWorkspace(),
      m_eMin(), m_eMax(), m_isPositiveReflect(), m_spectraRange() {}

void SymmetriseModel::setupPreviewAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                            std::vector<int> const &spectraRange) {

  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
  }
  // Run the algorithm on the preview spectrum only, these outputs are only for plotting in the preview window and are
  // not accessed by users directly.
  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflectedInputWorkspace);
  symmetriseAlg->setProperty("XMin", m_eMin);
  symmetriseAlg->setProperty("XMax", m_eMax);
  symmetriseAlg->setProperty("SpectraRange", spectraRange);
  symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");
  symmetriseAlg->setRethrows(true);

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
}

std::string SymmetriseModel::setupSymmetriseAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {

  std::string outputWorkspace = m_positiveOutputWorkspace;
  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
    outputWorkspace = m_negativeOutputWorkspace;
  }

  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflectedInputWorkspace);
  symmetriseAlg->setProperty("XMin", m_eMin);
  symmetriseAlg->setProperty("XMax", m_eMax);
  symmetriseAlg->setProperty("OutputWorkspace", outputWorkspace);
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
  return outputWorkspace;
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
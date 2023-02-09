// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSymmetriseTabModel.h"
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
InelasticDataManipulationSymmetriseTabModel::InelasticDataManipulationSymmetriseTabModel() {}

void InelasticDataManipulationSymmetriseTabModel::setupPreviewAlgorithm(
    MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::vector<long> spectraRange) {

  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
  }
  // Run the algorithm on the preview spectrum only, these outputs are only for plotting in the preview window and are
  // not accessed by users directly.
  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflctedInputWorkspace);
  symmetriseAlg->setProperty("XMin", m_eMin);
  symmetriseAlg->setProperty("XMax", m_eMax);
  symmetriseAlg->setProperty("SpectraRange", spectraRange);
  symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");
  symmetriseAlg->setRethrows(true);

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
}

std::string InelasticDataManipulationSymmetriseTabModel::setupSymmetriseAlgorithm(
    MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {

  std::string outputWorkspace = m_positiveOutputWorkspace;
  if (!m_isPositiveReflect) {
    reflectNegativeToPositive();
    outputWorkspace = m_negativeOutputWorkspace;
  }

  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise");
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", m_isPositiveReflect ? m_inputWorkspace : m_reflctedInputWorkspace);
  symmetriseAlg->setProperty("XMin", m_eMin);
  symmetriseAlg->setProperty("XMax", m_eMax);
  symmetriseAlg->setProperty("OutputWorkspace", outputWorkspace);
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
  return outputWorkspace;
}

void InelasticDataManipulationSymmetriseTabModel::reflectNegativeToPositive() {
  IAlgorithm_sptr scaleXAlg = AlgorithmManager::Instance().create("ScaleX");
  scaleXAlg->initialize();
  scaleXAlg->setProperty("InputWorkspace", m_inputWorkspace);
  scaleXAlg->setProperty("Operation", "Multiply");
  scaleXAlg->setProperty("Factor", -1.0);
  scaleXAlg->setProperty("OutputWorkspace", m_reflctedInputWorkspace);
  scaleXAlg->execute();

  IAlgorithm_sptr sortXAxisAlg = AlgorithmManager::Instance().create("SortXAxis");
  sortXAxisAlg->initialize();
  sortXAxisAlg->setProperty("InputWorkspace", m_reflctedInputWorkspace);
  sortXAxisAlg->setProperty("OutputWorkspace", m_reflctedInputWorkspace);
  sortXAxisAlg->execute();
}

void InelasticDataManipulationSymmetriseTabModel::setWorkspaceName(QString workspaceName) {
  m_inputWorkspace = workspaceName.toStdString();
  m_reflctedInputWorkspace = m_inputWorkspace + "_reflected";
  // the last 4 characters in the workspace name are '_red' the ouput weorkspace name is inserting '_sym' before that
  // '_red'
  m_positiveOutputWorkspace =
      (workspaceName.left(workspaceName.length() - 4) + "_sym" + "_pn" + workspaceName.right(4)).toStdString();
  m_negativeOutputWorkspace =
      (workspaceName.left(workspaceName.length() - 4) + "_sym" + "_np" + workspaceName.right(4)).toStdString();
}

void InelasticDataManipulationSymmetriseTabModel::setEMin(double value) { m_eMin = value; }

void InelasticDataManipulationSymmetriseTabModel::setEMax(double value) { m_eMax = value; }

void InelasticDataManipulationSymmetriseTabModel::setIsPositiveReflect(bool value) { m_isPositiveReflect = value; }

} // namespace MantidQt::CustomInterfaces
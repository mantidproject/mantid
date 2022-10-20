// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSymmetriseModel.h"
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
IndirectSymmetriseModel::IndirectSymmetriseModel() {}

void IndirectSymmetriseModel::setupPreviewAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                                    QString workspaceName, double eMin, double eMax,
                                                    std::vector<long> spectraRange) {
  // Run the algorithm on the preview spectrum only
  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", eMin);
  symmetriseAlg->setProperty("XMax", eMax);
  symmetriseAlg->setProperty("SpectraRange", spectraRange);
  symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");
  symmetriseAlg->setRethrows(true);

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
}

std::string IndirectSymmetriseModel::setupSymmetriseAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                                              QString workspaceName, double eMin, double eMax) {
  QString outputWorkspaceName = workspaceName.left(workspaceName.length() - 4) + "_sym" + workspaceName.right(4);

  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", eMin);
  symmetriseAlg->setProperty("XMax", eMax);
  symmetriseAlg->setProperty("OutputWorkspace", outputWorkspaceName.toStdString());
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  batchAlgoRunner->addAlgorithm(symmetriseAlg);
  return outputWorkspaceName.toStdString();
}

} // namespace MantidQt::CustomInterfaces
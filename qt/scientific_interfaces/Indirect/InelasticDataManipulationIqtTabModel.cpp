// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationIqtTabModel.h"
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
InelasticDataManipulationIqtTabModel::InelasticDataManipulationIqtTabModel() {}

void InelasticDataManipulationIqtTabModel::setupTransformToIqt(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                                               std::string const &outputWorkspace) {
  auto IqtAlg = AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();
  IqtAlg->setProperty("SampleWorkspace", m_sampleWorkspace);
  IqtAlg->setProperty("ResolutionWorkspace", m_resWorkspace);
  IqtAlg->setProperty("NumberOfIterations", m_nIterations);
  IqtAlg->setProperty("CalculateErrors", m_calculateErrors);

  IqtAlg->setProperty("EnergyMin", m_energyMin);
  IqtAlg->setProperty("EnergyMax", m_energyMax);
  IqtAlg->setProperty("BinReductionFactor", m_numBins);
  IqtAlg->setProperty("OutputWorkspace", outputWorkspace);

  IqtAlg->setProperty("DryRun", false);

  batchAlgoRunner->addAlgorithm(IqtAlg);
}

void InelasticDataManipulationIqtTabModel::setSampleWorkspace(std::string const &sampleWorkspace) {
  m_sampleWorkspace = sampleWorkspace;
}

void InelasticDataManipulationIqtTabModel::setResWorkspace(std::string const &resWorkspace) {
  m_resWorkspace = resWorkspace;
}

void InelasticDataManipulationIqtTabModel::setNIterations(std::string const &nIterations) {
  m_nIterations = nIterations;
}

void InelasticDataManipulationIqtTabModel::setEnergyMin(double energyMin) { m_energyMin = energyMin; }

void InelasticDataManipulationIqtTabModel::setEnergyMax(double energyMax) { m_energyMax = energyMax; }

void InelasticDataManipulationIqtTabModel::setNumBins(double numBins) { m_numBins = numBins; }

void InelasticDataManipulationIqtTabModel::setCalculateErrors(bool calculateErrors) {
  m_calculateErrors = calculateErrors;
}

} // namespace MantidQt::CustomInterfaces

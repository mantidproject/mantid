// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtModel.h"
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

IqtModel::IqtModel()
    : m_sampleWorkspace(), m_resWorkspace(), m_nIterations(), m_energyMin(), m_energyMax(), m_numBins(),
      m_calculateErrors(), m_enforceNormalization() {}

void IqtModel::setupTransformToIqt(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                   std::string const &outputWorkspace) {
  auto IqtAlg = AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();
  IqtAlg->setProperty("SampleWorkspace", m_sampleWorkspace);
  IqtAlg->setProperty("ResolutionWorkspace", m_resWorkspace);
  IqtAlg->setProperty("NumberOfIterations", m_nIterations);
  IqtAlg->setProperty("CalculateErrors", m_calculateErrors);
  IqtAlg->setProperty("EnforceNormalization", m_enforceNormalization);
  IqtAlg->setProperty("EnergyMin", m_energyMin);
  IqtAlg->setProperty("EnergyMax", m_energyMax);
  IqtAlg->setProperty("BinReductionFactor", m_numBins);
  IqtAlg->setProperty("OutputWorkspace", outputWorkspace);
  IqtAlg->setProperty("DryRun", false);

  batchAlgoRunner->addAlgorithm(IqtAlg);
}

void IqtModel::setSampleWorkspace(std::string const &sampleWorkspace) { m_sampleWorkspace = sampleWorkspace; }

void IqtModel::setResWorkspace(std::string const &resWorkspace) { m_resWorkspace = resWorkspace; }

void IqtModel::setNIterations(std::string const &nIterations) { m_nIterations = nIterations; }

void IqtModel::setEnergyMin(double energyMin) { m_energyMin = energyMin; }

void IqtModel::setEnergyMax(double energyMax) { m_energyMax = energyMax; }

void IqtModel::setNumBins(double numBins) { m_numBins = numBins; }

void IqtModel::setCalculateErrors(bool calculateErrors) { m_calculateErrors = calculateErrors; }

void IqtModel::setEnforceNormalization(bool enforceNormalization) { m_enforceNormalization = enforceNormalization; }

double IqtModel::EMin() const { return m_energyMin; }

double IqtModel::EMax() const { return m_energyMax; }

} // namespace MantidQt::CustomInterfaces

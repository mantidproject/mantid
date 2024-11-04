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

#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>
#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

IqtModel::IqtModel()
    : m_sampleWorkspace(), m_resWorkspace(), m_nIterations(), m_energyMin(), m_energyMax(), m_numBins(),
      m_calculateErrors(), m_enforceNormalization() {}

API::IConfiguredAlgorithm_sptr IqtModel::setupTransformToIqt(std::string const &outputWorkspace) const {
  auto IqtAlg = AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  properties->setProperty("ResolutionWorkspace", m_resWorkspace);
  properties->setProperty("NumberOfIterations", m_nIterations);
  properties->setProperty("CalculateErrors", m_calculateErrors);
  properties->setProperty("EnforceNormalization", m_enforceNormalization);
  properties->setProperty("EnergyMin", m_energyMin);
  properties->setProperty("EnergyMax", m_energyMax);
  properties->setProperty("BinReductionFactor", m_numBins);
  properties->setProperty("OutputWorkspace", outputWorkspace);
  properties->setProperty("DryRun", false);
  properties->setProperty("SampleWorkspace", m_sampleWorkspace);
  MantidQt::API::IConfiguredAlgorithm_sptr confAlg =
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(IqtAlg, std::move(properties));

  return confAlg;
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

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSqwTabModel.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &tuple) {
  return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
}

void convertToSpectrumAxis(std::string const &inputName, std::string const &outputName) {
  auto converter = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  converter->initialize();
  converter->setProperty("InputWorkspace", inputName);
  converter->setProperty("OutputWorkspace", outputName);
  converter->setProperty("Target", "ElasticQ");
  converter->setProperty("EMode", "Indirect");
  converter->execute();
}
} // namespace

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InelasticDataManipulationSqwTabModel::InelasticDataManipulationSqwTabModel() { m_rebinInEnergy = false; }

void InelasticDataManipulationSqwTabModel::setupRebinAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
  if (m_rebinInEnergy) {
    std::string eRebinString = std::to_string(m_eLow) + "," + std::to_string(m_eWidth) + "," + std::to_string(m_eHigh);
    auto const eRebinWsName = m_baseName + "_r";
    auto energyRebinAlg = AlgorithmManager::Instance().create("Rebin");
    energyRebinAlg->initialize();
    energyRebinAlg->setProperty("InputWorkspace", m_inputWorkspace);
    energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName);
    energyRebinAlg->setProperty("Params", eRebinString);
    batchAlgoRunner->addAlgorithm(energyRebinAlg);
  }
}

void InelasticDataManipulationSqwTabModel::setupSofQWAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
  std::string qRebinString = std::to_string(m_qLow) + "," + std::to_string(m_qWidth) + "," + std::to_string(m_qHigh);

  auto const sqwWsName = getOutputWorkspace();
  auto const eRebinWsName = m_baseName + "_r";

  auto sqwAlg = AlgorithmManager::Instance().create("SofQW");
  sqwAlg->initialize();
  sqwAlg->setProperty("OutputWorkspace", sqwWsName);
  sqwAlg->setProperty("QAxisBinning", qRebinString);
  sqwAlg->setProperty("EMode", "Indirect");
  sqwAlg->setProperty("EFixed", m_eFixed);
  sqwAlg->setProperty("Method", "NormalisedPolygon");
  sqwAlg->setProperty("ReplaceNaNs", true);

  auto sqwInputProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  sqwInputProps->setPropertyValue("InputWorkspace", m_rebinInEnergy ? eRebinWsName : m_inputWorkspace);

  batchAlgoRunner->addAlgorithm(sqwAlg, std::move(sqwInputProps));
}

void InelasticDataManipulationSqwTabModel::setupAddSampleLogAlgorithm(
    MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
  auto const sqwWsName = getOutputWorkspace();
  // Add sample log for S(Q, w) algorithm used
  auto sampleLogAlg = AlgorithmManager::Instance().create("AddSampleLog");
  sampleLogAlg->initialize();
  sampleLogAlg->setProperty("LogName", "rebin_type");
  sampleLogAlg->setProperty("LogType", "String");
  sampleLogAlg->setProperty("LogText", "NormalisedPolygon");

  auto inputToAddSampleLogProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  inputToAddSampleLogProps->setPropertyValue("Workspace", sqwWsName);

  batchAlgoRunner->addAlgorithm(sampleLogAlg, std::move(inputToAddSampleLogProps));
}

void InelasticDataManipulationSqwTabModel::setInputWorkspace(const std::string &workspace) {
  m_inputWorkspace = workspace;
  // remove _red suffix from inpur workspace for outputs
  m_baseName = m_inputWorkspace.substr(0, m_inputWorkspace.find("_red", m_inputWorkspace.length() - 4));
}

void InelasticDataManipulationSqwTabModel::setQMin(double qMin) { m_qLow = qMin; }

void InelasticDataManipulationSqwTabModel::setQWidth(double qWidth) { m_qWidth = qWidth; }

void InelasticDataManipulationSqwTabModel::setQMax(double qMax) { m_qHigh = qMax; }

void InelasticDataManipulationSqwTabModel::setEMin(double eMin) { m_eLow = eMin; }

void InelasticDataManipulationSqwTabModel::setEWidth(double eWidth) { m_eWidth = eWidth; }

void InelasticDataManipulationSqwTabModel::setEMax(double eMax) { m_eHigh = eMax; }

void InelasticDataManipulationSqwTabModel::setEFixed(const double eFixed) { m_eFixed = eFixed; }

void InelasticDataManipulationSqwTabModel::setRebinInEnergy(bool scale) { m_rebinInEnergy = scale; }

std::string InelasticDataManipulationSqwTabModel::getOutputWorkspace() { return m_baseName + "_sqw"; }

MatrixWorkspace_sptr InelasticDataManipulationSqwTabModel::getRqwWorkspace() {
  auto const outputName = m_inputWorkspace.substr(0, m_inputWorkspace.size() - 4) + "_rqw";
  convertToSpectrumAxis(m_inputWorkspace, outputName);
  return getADSMatrixWorkspace(outputName);
}

UserInputValidator InelasticDataManipulationSqwTabModel::validate(std::tuple<double, double> const qRange,
                                                                  std::tuple<double, double> const eRange) {

  double const tolerance = 1e-10;

  UserInputValidator uiv;

  // Validate Q binning
  uiv.checkBins(m_qLow, m_qWidth, m_qHigh, tolerance);
  uiv.checkRangeIsEnclosed("The contour plots Q axis", convertTupleToPair(qRange), "the Q range provided",
                           std::make_pair(m_qLow, m_qHigh));

  // If selected, validate energy binning
  if (m_rebinInEnergy) {

    uiv.checkBins(m_eLow, m_eWidth, m_eHigh, tolerance);
    uiv.checkRangeIsEnclosed("The contour plots Energy axis", convertTupleToPair(eRange), "the E range provided",
                             std::make_pair(m_eLow, m_eHigh));
  }
  return uiv;
}

std::string InelasticDataManipulationSqwTabModel::getEFixedFromInstrument(std::string const &instrumentName,
                                                                          std::string analyser,
                                                                          std::string const &reflection) {

  // In the IRIS IPF there is no fmica component
  if (instrumentName == "IRIS" && analyser == "fmica")
    analyser = "mica";

  // Get the instrument
  auto const instWorkspace = loadInstrumentWorkspace(instrumentName, analyser, reflection);

  auto const instrument = instWorkspace->getInstrument();

  // Get the analyser component
  auto const component = instrument->getComponentByName(analyser);
  std::string eFixedValue = "";
  if (instrument->hasParameter("Efixed")) {
    eFixedValue = std::to_string(instrument->getNumberParameter("Efixed")[0]);
  }
  if (eFixedValue.empty() && component != nullptr)
    eFixedValue = std::to_string(component->getNumberParameter("Efixed")[0]);
  return eFixedValue;
}

/**
 * Loads an empty instrument into a workspace and returns a pointer to it.
 *
 * If an analyser and reflection are supplied then the corresponding IPF is also
 *loaded.
 * The workspace is not stored in ADS.
 *
 * @param instrumentName Name of the instrument to load
 * @param analyser Analyser being used (optional)
 * @param reflection Relection being used (optional)
 */
MatrixWorkspace_sptr InelasticDataManipulationSqwTabModel::loadInstrumentWorkspace(const std::string &instrumentName,
                                                                                   const std::string &analyser,
                                                                                   const std::string &reflection) {
  std::string idfdirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  auto const ipfFilename = idfdirectory + instrumentName + "_" + analyser + "_" + reflection + "_Parameters.xml";

  auto const dateRange = instrumentName == "BASIS" ? "_2014-2018" : "";
  auto const parameterFilename = idfdirectory + instrumentName + "_Definition" + dateRange + ".xml";
  auto loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  loadAlg->initialize();
  loadAlg->setProperty("Filename", parameterFilename);
  loadAlg->setProperty("OutputWorkspace", "__IDR_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");

  // Load the IPF if given an analyser and reflection
  if (!analyser.empty() && !reflection.empty()) {
    auto loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->setChild(true);
    loadParamAlg->setLogging(false);
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Filename", ipfFilename);
    loadParamAlg->setProperty("Workspace", instWorkspace);
    loadParamAlg->execute();
  }
  return instWorkspace;
}

} // namespace MantidQt::CustomInterfaces

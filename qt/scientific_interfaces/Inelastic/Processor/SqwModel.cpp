// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SqwModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
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

SqwModel::SqwModel()
    : m_inputWorkspace(), m_baseName(), m_eFixed(), m_qLow(), m_qWidth(0.05), m_qHigh(), m_eLow(), m_eWidth(0.005),
      m_eHigh(), m_rebinInEnergy(false) {}

void SqwModel::setupRebinAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
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

void SqwModel::setupSofQWAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
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

  auto sqwInputProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  sqwInputProps->setPropertyValue("InputWorkspace", m_rebinInEnergy ? eRebinWsName : m_inputWorkspace);

  batchAlgoRunner->addAlgorithm(sqwAlg, std::move(sqwInputProps));
}

void SqwModel::setupAddSampleLogAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
  auto const sqwWsName = getOutputWorkspace();
  // Add sample log for S(Q, w) algorithm used
  auto sampleLogAlg = AlgorithmManager::Instance().create("AddSampleLog");
  sampleLogAlg->initialize();
  sampleLogAlg->setProperty("LogName", "rebin_type");
  sampleLogAlg->setProperty("LogType", "String");
  sampleLogAlg->setProperty("LogText", "NormalisedPolygon");

  auto inputToAddSampleLogProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  inputToAddSampleLogProps->setPropertyValue("Workspace", sqwWsName);

  batchAlgoRunner->addAlgorithm(sampleLogAlg, std::move(inputToAddSampleLogProps));
}

void SqwModel::setInputWorkspace(const std::string &workspace) {
  m_inputWorkspace = workspace;
  // remove _red suffix from inpur workspace for outputs
  m_baseName = m_inputWorkspace.substr(0, m_inputWorkspace.find("_red", m_inputWorkspace.length() - 4));
}

void SqwModel::setQMin(double qMin) { m_qLow = qMin; }

void SqwModel::setQWidth(double qWidth) { m_qWidth = qWidth; }

void SqwModel::setQMax(double qMax) { m_qHigh = qMax; }

void SqwModel::setEMin(double eMin) { m_eLow = eMin; }

void SqwModel::setEWidth(double eWidth) { m_eWidth = eWidth; }

void SqwModel::setEMax(double eMax) { m_eHigh = eMax; }

void SqwModel::setEFixed(const double eFixed) { m_eFixed = eFixed; }

void SqwModel::setRebinInEnergy(bool scale) { m_rebinInEnergy = scale; }

std::string SqwModel::getOutputWorkspace() const { return m_baseName + "_sqw"; }

MatrixWorkspace_sptr SqwModel::getRqwWorkspace() const {
  auto const outputName = m_inputWorkspace.substr(0, m_inputWorkspace.size() - 4) + "_rqw";
  convertToSpectrumAxis(m_inputWorkspace, outputName);
  return getADSWorkspace(outputName);
}

void SqwModel::validate(IUserInputValidator *validator, std::tuple<double, double> const qRange,
                        std::tuple<double, double> const eRange) const {

  double const tolerance = 1e-10;

  // Validate Q binning
  validator->checkBins(m_qLow, m_qWidth, m_qHigh, tolerance);
  validator->checkRangeIsEnclosed("The contour plots Q axis", convertTupleToPair(qRange), "the Q range provided",
                                  std::make_pair(m_qLow, m_qHigh));

  // If selected, validate energy binning
  if (m_rebinInEnergy) {

    validator->checkBins(m_eLow, m_eWidth, m_eHigh, tolerance);
    validator->checkRangeIsEnclosed("The contour plots Energy axis", convertTupleToPair(eRange), "the E range provided",
                                    std::make_pair(m_eLow, m_eHigh));
  }
}

std::string SqwModel::getEFixedFromInstrument(std::string const &instrumentName, std::string analyser,
                                              std::string const &reflection) const {

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
MatrixWorkspace_sptr SqwModel::loadInstrumentWorkspace(const std::string &instrumentName, const std::string &analyser,
                                                       const std::string &reflection) const {
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

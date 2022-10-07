// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqwModel.h"
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

namespace {

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &tuple) {
  return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
}

double roundToPrecision(double value, double precision) { return value - std::remainder(value, precision); }

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
IndirectSqwModel::IndirectSqwModel() { m_rebinInEnergy = false; }

void IndirectSqwModel::setupRebinAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
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

void IndirectSqwModel::setupSofQWAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
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

void IndirectSqwModel::setupAddSampleLogAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner) {
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

void IndirectSqwModel::setInputWorkspace(const std::string &workspace) {
  m_inputWorkspace = workspace;
  // remove _red suffix from inpur workspace for outputs
  m_baseName = m_inputWorkspace.substr(0, m_inputWorkspace.find("_red", m_inputWorkspace.length() - 4));
}

void IndirectSqwModel::setQMin(double qMin) { m_qLow = qMin; }

void IndirectSqwModel::setQWidth(double qWidth) { m_qWidth = qWidth; }

void IndirectSqwModel::setQMax(double qMax) { m_qHigh = qMax; }

void IndirectSqwModel::setEMin(double eMin) { m_eLow = eMin; }

void IndirectSqwModel::setEWidth(double eWidth) { m_eWidth = eWidth; }

void IndirectSqwModel::setEMax(double eMax) { m_eHigh = eMax; }

void IndirectSqwModel::setEFixed(const std::string &eFixed) { m_eFixed = eFixed; }

void IndirectSqwModel::setRebinInEnergy(bool scale) { m_rebinInEnergy = scale; }

std::string IndirectSqwModel::getOutputWorkspace() { return m_baseName + "_sqw"; }

MatrixWorkspace_sptr IndirectSqwModel::getRqwWorkspace() {
  auto const outputName = m_inputWorkspace.substr(0, m_inputWorkspace.size() - 4) + "_rqw";
  convertToSpectrumAxis(m_inputWorkspace, outputName);
  return getADSMatrixWorkspace(outputName);
}

UserInputValidator IndirectSqwModel::validate(std::tuple<double, double> const qRange,
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

std::pair<double, double> IndirectSqwModel::roundToWidth(std::tuple<double, double> const &axisRange, double width) {
  return std::make_pair(roundToPrecision(std::get<0>(axisRange), width) + width,
                        roundToPrecision(std::get<1>(axisRange), width) - width);
}

} // namespace MantidQt::CustomInterfaces

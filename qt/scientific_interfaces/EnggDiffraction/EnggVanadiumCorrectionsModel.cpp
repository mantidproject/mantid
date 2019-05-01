// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggVanadiumCorrectionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <Poco/File.h>

namespace MantidQt {
namespace CustomInterfaces {

EnggVanadiumCorrectionsModel::EnggVanadiumCorrectionsModel(
    const EnggDiffCalibSettings &calibSettings,
    const std::string &currentInstrument)
    : m_calibSettings(calibSettings), m_currentInstrument(currentInstrument) {}

const std::string EnggVanadiumCorrectionsModel::CURVES_WORKSPACE_NAME =
    "engggui_vanadium_curves";

const std::string EnggVanadiumCorrectionsModel::INTEGRATED_WORKSPACE_NAME =
    "engggui_vanadium_integration";

const std::string EnggVanadiumCorrectionsModel::VANADIUM_INPUT_WORKSPACE_NAME =
    "engggui_vanadium_ws";

std::pair<Mantid::API::ITableWorkspace_sptr, Mantid::API::MatrixWorkspace_sptr>
EnggVanadiumCorrectionsModel::calculateCorrectionWorkspaces(
    const std::string &vanadiumRunNumber) const {
  const auto vanadiumRunName = generateVanadiumRunName(vanadiumRunNumber);
  loadMatrixWorkspace(vanadiumRunName, VANADIUM_INPUT_WORKSPACE_NAME);
  auto enggVanadiumCorrections =
      Mantid::API::AlgorithmManager::Instance().create(
          "EnggVanadiumCorrections");
  enggVanadiumCorrections->initialize();
  enggVanadiumCorrections->setPropertyValue("VanadiumWorkspace",
                                            VANADIUM_INPUT_WORKSPACE_NAME);
  enggVanadiumCorrections->setPropertyValue("OutIntegrationWorkspace",
                                            INTEGRATED_WORKSPACE_NAME);
  enggVanadiumCorrections->setPropertyValue("OutCurvesWorkspace",
                                            CURVES_WORKSPACE_NAME);
  enggVanadiumCorrections->execute();

  auto &ADS = Mantid::API::AnalysisDataService::Instance();

  ADS.remove(VANADIUM_INPUT_WORKSPACE_NAME);
  const auto integratedWorkspace =
      ADS.retrieveWS<Mantid::API::ITableWorkspace>(INTEGRATED_WORKSPACE_NAME);
  const auto curvesWorkspace =
      ADS.retrieveWS<Mantid::API::MatrixWorkspace>(CURVES_WORKSPACE_NAME);
  return std::make_pair(integratedWorkspace, curvesWorkspace);
}

std::pair<Mantid::API::ITableWorkspace_sptr, Mantid::API::MatrixWorkspace_sptr>
EnggVanadiumCorrectionsModel::fetchCorrectionWorkspaces(
    const std::string &vanadiumRunNumber) const {
  const auto cachedCurvesWorkspace =
      fetchCachedCurvesWorkspace(vanadiumRunNumber);
  const auto cachedIntegratedWorkspace =
      fetchCachedIntegratedWorkspace(vanadiumRunNumber);

  if (cachedCurvesWorkspace && cachedIntegratedWorkspace &&
      !m_calibSettings.m_forceRecalcOverwrite) {
    return std::make_pair(cachedIntegratedWorkspace, cachedCurvesWorkspace);
  } else {
    const auto correctionWorkspaces =
        calculateCorrectionWorkspaces(vanadiumRunNumber);
    saveCorrectionsToCache(vanadiumRunNumber, correctionWorkspaces.second,
                           correctionWorkspaces.first);
    return correctionWorkspaces;
  }
}

Mantid::API::MatrixWorkspace_sptr
EnggVanadiumCorrectionsModel::fetchCachedCurvesWorkspace(
    const std::string &vanadiumRunNumber) const {
  const auto filename = generateCurvesFilename(vanadiumRunNumber);
  const Poco::File inputFile(filename);
  if (inputFile.exists()) {
    return loadMatrixWorkspace(filename, CURVES_WORKSPACE_NAME);
  }

  return nullptr;
}

Mantid::API::ITableWorkspace_sptr
EnggVanadiumCorrectionsModel::fetchCachedIntegratedWorkspace(
    const std::string &vanadiumRunNumber) const {
  const auto filename = generateIntegratedFilename(vanadiumRunNumber);
  const Poco::File inputFile(filename);
  if (inputFile.exists()) {
    return loadTableWorkspace(filename, INTEGRATED_WORKSPACE_NAME);
  }

  return nullptr;
}

std::string EnggVanadiumCorrectionsModel::generateCurvesFilename(
    const std::string &vanadiumRunNumber) const {
  const static std::string filenameSuffix =
      "_precalculated_vanadium_run_bank_curves.nxs";

  const auto normalisedRunName = generateVanadiumRunName(vanadiumRunNumber);
  const auto curvesFilename = normalisedRunName + filenameSuffix;

  Poco::Path inputDir(m_calibSettings.m_inputDirCalib);
  inputDir.append(curvesFilename);
  return inputDir.toString();
}

std::string EnggVanadiumCorrectionsModel::generateIntegratedFilename(
    const std::string &vanadiumRunNumber) const {
  const static std::string filenameSuffix =
      "_precalculated_vanadium_run_integration.nxs";

  const auto normalisedRunName = generateVanadiumRunName(vanadiumRunNumber);
  const auto integratedFilename = normalisedRunName + filenameSuffix;

  Poco::Path inputDir(m_calibSettings.m_inputDirCalib);
  inputDir.append(integratedFilename);
  return inputDir.toString();
}

std::string EnggVanadiumCorrectionsModel::generateVanadiumRunName(
    const std::string &vanadiumRunNumber) const {
  constexpr static size_t normalisedRunNumberLength = 8;
  return m_currentInstrument +
         std::string(normalisedRunNumberLength - vanadiumRunNumber.length(),
                     '0') +
         vanadiumRunNumber;
}

Mantid::API::MatrixWorkspace_sptr
EnggVanadiumCorrectionsModel::loadMatrixWorkspace(
    const std::string &filename, const std::string &workspaceName) const {
  auto load = Mantid::API::AlgorithmManager::Instance().create("Load");
  load->initialize();
  load->setPropertyValue("Filename", filename);
  load->setPropertyValue("OutputWorkspace", workspaceName);
  load->execute();
  return Mantid::API::AnalysisDataService::Instance()
      .retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
}

Mantid::API::ITableWorkspace_sptr
EnggVanadiumCorrectionsModel::loadTableWorkspace(
    const std::string &filename, const std::string &workspaceName) const {
  auto load = Mantid::API::AlgorithmManager::Instance().create("Load");
  load->initialize();
  load->setPropertyValue("Filename", filename);
  load->setPropertyValue("OutputWorkspace", workspaceName);
  load->execute();
  return Mantid::API::AnalysisDataService::Instance()
      .retrieveWS<Mantid::API::ITableWorkspace>(workspaceName);
}

void EnggVanadiumCorrectionsModel::saveCorrectionsToCache(
    const std::string &runNumber,
    const Mantid::API::MatrixWorkspace_sptr curvesWorkspace,
    const Mantid::API::ITableWorkspace_sptr integratedWorkspace) const {
  const auto curvesFilename = generateCurvesFilename(runNumber);
  saveNexus(curvesFilename, curvesWorkspace);

  const auto integratedFilename = generateIntegratedFilename(runNumber);
  saveNexus(integratedFilename, integratedWorkspace);
}

void EnggVanadiumCorrectionsModel::saveNexus(
    const std::string &filename,
    const Mantid::API::Workspace_sptr workspace) const {
  auto save = Mantid::API::AlgorithmManager::Instance().create("SaveNexus");
  save->initialize();
  save->setProperty("InputWorkspace", workspace);
  save->setProperty("Filename", filename);
  save->execute();
}

void EnggVanadiumCorrectionsModel::setCalibSettings(
    const EnggDiffCalibSettings &calibSettings) {
  m_calibSettings = calibSettings;
}

void EnggVanadiumCorrectionsModel::setCurrentInstrument(
    const std::string &currentInstrument) {
  m_currentInstrument = currentInstrument;
}

} // namespace CustomInterfaces
} // namespace MantidQt

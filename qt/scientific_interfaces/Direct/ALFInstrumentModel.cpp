// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentModel.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

#include <memory>
#include <utility>

using namespace Mantid::API;

namespace {
auto &ADS = AnalysisDataService::Instance();

std::string const CURVES = "Curves";
std::string const EXTRACTED_PREFIX = "extractedTubes_";
std::string const D_SPACING_UNIT = "dSpacing";
std::string const PHI_UNIT = "Phi";
std::string const OUT_OF_PLANE_ANGLE_LABEL = "Out of plane angle";
std::string const NOT_IN_ADS = "not_stored_in_ads";

void loadEmptyInstrument(std::string const &instrumentName, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("InstrumentName", instrumentName);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

MatrixWorkspace_sptr load(std::string const &filename) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  Workspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace);
}

void rebinToWorkspace(std::string const &workspaceToRebin, MatrixWorkspace_sptr &workspaceToMatch,
                      std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("RebinToWorkspace");
  alg->initialize();
  alg->setProperty("WorkspaceToRebin", workspaceToRebin);
  alg->setProperty("WorkspaceToMatch", workspaceToMatch);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

void plus(std::string const &lhsWorkspace, MatrixWorkspace_sptr &rhsWorkspace, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("Plus");
  alg->initialize();
  alg->setProperty("LHSWorkspace", lhsWorkspace);
  alg->setProperty("RHSWorkspace", rhsWorkspace);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

MatrixWorkspace_sptr normaliseByCurrent(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr convertUnits(MatrixWorkspace_sptr const &inputWorkspace, std::string const &target) {
  auto alg = AlgorithmManager::Instance().create("ConvertUnits");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Target", target);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

void scaleX(std::string const &inputName, double const factor, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("ScaleX");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputName);
  alg->setProperty("Factor", factor);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

void convertToHistogram(std::string const &inputName, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("ConvertToHistogram");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputName);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel()
    : m_currentRun(0), m_tmpName("ALF_tmp"), m_instrumentName("ALF"), m_wsName("ALFData"), m_numberOfTubesInAverage(0) {
  loadEmptyInstrument(m_instrumentName, m_wsName);
}

/*
 * Loads data into the ALFView. Normalises and converts to DSpacing if necessary.
 * @param filename:: The filepath to the ALFData
 * @return An optional error message
 */
std::optional<std::string> ALFInstrumentModel::loadData(std::string const &filename) {
  auto loadedWorkspace = load(filename);

  if (!isALFData(loadedWorkspace)) {
    return "Not the correct instrument, expected " + m_instrumentName;
  }

  m_numberOfTubesInAverage = 0;
  m_currentRun = loadedWorkspace->getRunNumber();

  if (!isAxisDSpacing(loadedWorkspace)) {
    convertUnits(normaliseByCurrent(loadedWorkspace), D_SPACING_UNIT);
  }

  ADS.addOrReplace(m_wsName, loadedWorkspace);
  return std::nullopt;
}

void ALFInstrumentModel::averageTube() {
  auto name = m_instrumentName + std::to_string(m_currentRun);
  auto extractedName = extractedWsName();
  const int oldTotalNumber = m_numberOfTubesInAverage;
  // multiply up current average
  auto ws = ADS.retrieveWS<MatrixWorkspace>(extractedName);
  ws *= double(oldTotalNumber);

  // get the data to add
  storeSingleTube(name);
  // rebin to match
  rebinToWorkspace(extractedName, ws, extractedName);
  // add together
  plus(extractedName, ws, extractedName);

  // do division
  ws = ADS.retrieveWS<MatrixWorkspace>(extractedName);
  ws->mutableY(0) /= (double(oldTotalNumber) + 1.0);
  ADS.addOrReplace(extractedName, ws);
  m_numberOfTubesInAverage++;
}

void ALFInstrumentModel::extractSingleTube() {
  storeSingleTube(m_instrumentName + std::to_string(m_currentRun));
  m_numberOfTubesInAverage = 1;
}

void ALFInstrumentModel::storeSingleTube(const std::string &name) {
  if (!ADS.doesExist(CURVES))
    return;

  const auto scaleFactor = xConversionFactor(ADS.retrieveWS<MatrixWorkspace>(CURVES));
  if (!scaleFactor)
    return;

  auto extractedName = extractedWsName();
  // Convert to degrees if the XAxis is an angle in radians
  scaleX(CURVES, *scaleFactor, extractedName);

  convertToHistogram(extractedName, extractedName);

  ADS.remove(CURVES);
}

bool ALFInstrumentModel::isALFData(MatrixWorkspace_const_sptr const &workspace) const {
  return workspace->getInstrument()->getName() == m_instrumentName;
}

bool ALFInstrumentModel::isAxisDSpacing(MatrixWorkspace_const_sptr const &workspace) const {
  return workspace->getAxis(0)->unit()->unitID() == D_SPACING_UNIT;
}

/*
 * Returns a conversion factor to be used for ScaleX when the x axis unit is an angle measured in radians. If
 * the x axis unit is not 'Phi' or 'Out of angle plane', no scaling is required.
 * @param workspace:: the workspace to check if a conversion factor is required.
 */
std::optional<double> ALFInstrumentModel::xConversionFactor(MatrixWorkspace_const_sptr workspace) const {
  if (!workspace)
    return std::nullopt;

  if (const auto axis = workspace->getAxis(0)) {
    const auto unit = axis->unit()->unitID();
    const auto label = std::string(axis->unit()->label());
    return unit == PHI_UNIT || label == OUT_OF_PLANE_ANGLE_LABEL ? 180.0 / M_PI : 1.0;
  }
  return std::nullopt;
}

std::string ALFInstrumentModel::extractedWsName() const {
  return EXTRACTED_PREFIX + m_instrumentName + std::to_string(m_currentRun);
}

bool ALFInstrumentModel::hasTubeBeenExtracted() const { return ADS.doesExist(extractedWsName()); }

int ALFInstrumentModel::numberOfTubesInAverage() const { return m_numberOfTubesInAverage; }

} // namespace MantidQt::CustomInterfaces

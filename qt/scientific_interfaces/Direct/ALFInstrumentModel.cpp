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
std::string const OUT_OF_PLANE_ANGLE_LABEL = "Out of plane angle";
std::string const NOT_IN_ADS = "not_stored_in_ads";

bool isALFData(MatrixWorkspace_const_sptr const &workspace) { return workspace->getInstrument()->getName() == "ALF"; }

bool isAxisDSpacing(MatrixWorkspace_const_sptr const &workspace) {
  return workspace->getAxis(0)->unit()->unitID() == "dSpacing";
}

std::optional<double> xConversionFactor(MatrixWorkspace_const_sptr const &workspace) {
  if (!workspace)
    return std::nullopt;

  if (const auto axis = workspace->getAxis(0)) {
    const auto unit = axis->unit()->unitID();
    const auto label = std::string(axis->unit()->label());
    return unit == "Phi" || label == "Out of plane angle" ? 180.0 / M_PI : 1.0;
  }
  return std::nullopt;
}

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

MatrixWorkspace_sptr rebinToWorkspace(MatrixWorkspace_sptr const &workspaceToRebin,
                                      MatrixWorkspace_sptr const &workspaceToMatch) {
  auto alg = AlgorithmManager::Instance().create("RebinToWorkspace");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("WorkspaceToRebin", workspaceToRebin);
  alg->setProperty("WorkspaceToMatch", workspaceToMatch);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr plus(MatrixWorkspace_sptr const &lhsWorkspace, MatrixWorkspace_sptr const &rhsWorkspace) {
  auto alg = AlgorithmManager::Instance().create("Plus");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("LHSWorkspace", lhsWorkspace);
  alg->setProperty("RHSWorkspace", rhsWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
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

MatrixWorkspace_sptr scaleX(MatrixWorkspace_sptr const &inputWorkspace, double const factor) {
  auto alg = AlgorithmManager::Instance().create("ScaleX");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Factor", factor);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr convertToHistogram(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ConvertToHistogram");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel()
    : m_currentRun(0), m_instrumentName("ALF"), m_wsName("ALFData"), m_numberOfTubesInAverage(0) {
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
    loadedWorkspace = convertUnits(normaliseByCurrent(loadedWorkspace), "dSpacing");
  }

  ADS.addOrReplace(m_wsName, loadedWorkspace);
  return std::nullopt;
}

/*
 * Extracts a single tube. Increments the average counter.
 */
void ALFInstrumentModel::extractSingleTube() {
  if (auto const extractedWorkspace = retrieveSingleTube()) {
    ADS.addOrReplace(extractedWsName(), extractedWorkspace);
    m_numberOfTubesInAverage = 1;
  }
}

MatrixWorkspace_sptr ALFInstrumentModel::retrieveSingleTube() {
  if (!ADS.doesExist(CURVES))
    return nullptr;

  // Get a handle on the curve workspace and then delete it from the ADS
  auto const curveWorkspace = ADS.retrieveWS<MatrixWorkspace>(CURVES);
  ADS.remove(CURVES);

  if (auto const scaleFactor = xConversionFactor(curveWorkspace)) {
    // Convert to degrees if the XAxis is an angle in radians, and then convert to histograms.
    return convertToHistogram(scaleX(curveWorkspace, *scaleFactor));
  }
  return nullptr;
}

void ALFInstrumentModel::averageTube() {
  // Multiply up the current average
  auto existingAverageTube = ADS.retrieveWS<MatrixWorkspace>(extractedWsName());
  existingAverageTube *= double(m_numberOfTubesInAverage);

  // Get the recently selected tube, and rebin to match previous extracted workspace
  auto newExtractedWorkspace = retrieveSingleTube();
  newExtractedWorkspace = rebinToWorkspace(newExtractedWorkspace, existingAverageTube);

  // Do an average
  auto averagedWorkspace = plus(newExtractedWorkspace, existingAverageTube);
  m_numberOfTubesInAverage++;
  averagedWorkspace->mutableY(0) /= double(m_numberOfTubesInAverage);

  // Add the result back into the ADS
  ADS.addOrReplace(extractedWsName(), averagedWorkspace);
}

std::string ALFInstrumentModel::extractedWsName() const {
  return EXTRACTED_PREFIX + m_instrumentName + std::to_string(m_currentRun);
}

bool ALFInstrumentModel::hasTubeBeenExtracted() const { return ADS.doesExist(extractedWsName()); }

int ALFInstrumentModel::numberOfTubesInAverage() const { return m_numberOfTubesInAverage; }

} // namespace MantidQt::CustomInterfaces

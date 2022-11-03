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
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

#include <utility>

using namespace Mantid::API;

namespace {
const int ERRORCODE = -999;
const std::string EXTRACTEDWS = "extractedTubes_";
const std::string CURVES = "Curves";

void loadEmptyInstrument(std::string const &instrumentName, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("InstrumentName", instrumentName);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void load(std::string const &filename, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void rebinToWorkspace(std::string const &workspaceToRebin, MatrixWorkspace_sptr &workspaceToMatch,
                      std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("RebinToWorkspace");
  alg->initialize();
  alg->setProperty("WorkspaceToRebin", workspaceToRebin);
  alg->setProperty("WorkspaceToMatch", workspaceToMatch);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void plus(std::string const &lhsWorkspace, MatrixWorkspace_sptr &rhsWorkspace, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("Plus");
  alg->initialize();
  alg->setProperty("LHSWorkspace", lhsWorkspace);
  alg->setProperty("RHSWorkspace", rhsWorkspace);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void normaliseByCurrent(std::string const &inputWorkspace, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void convertUnits(std::string const &inputWorkspace, std::string const &target, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ConvertUnits");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Target", target);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void scaleX(std::string const &inputWorkspace, double const factor, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ScaleX");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Factor", factor);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

void convertToHistogram(std::string const &inputWorkspace, std::string const &outputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ConvertToHistogram");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", outputWorkspace);
  alg->execute();
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel()
    : m_currentRun(0), m_tmpName("ALF_tmp"), m_instrumentName("ALF"), m_wsName("ALFData"), m_numberOfTubesInAverage(0) {
  loadEmptyInstrument(m_instrumentName, m_wsName);
}

/*
 * Loads data for use in ALFView
 * Loads data, normalise to current and then converts to d spacing
 * @param name:: string name for ALF data
 * @return std::pair<int,std::string>:: the run number and status
 */
std::pair<int, std::string> ALFInstrumentModel::loadData(const std::string &name) {
  load(name, getTmpName());
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getTmpName());
  int runNumber = ws->getRunNumber();
  std::string message = "success";
  auto bools = isDataValid();
  if (bools["IsValidInstrument"]) {
    rename();
    m_numberOfTubesInAverage = 0;
  } else {
    // reset to the previous data
    message = "Not the correct instrument, expected " + getInstrument();
    remove();
  }
  if (bools["IsValidInstrument"] && !bools["IsItDSpace"]) {
    transformData();
  }
  return std::make_pair(runNumber, message);
}

void ALFInstrumentModel::averageTube() {
  const std::string name = getInstrument() + std::to_string(getCurrentRun());
  const int oldTotalNumber = m_numberOfTubesInAverage;
  // multiply up current average
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(EXTRACTEDWS + name);
  ws *= double(oldTotalNumber);

  // get the data to add
  storeSingleTube(name);
  // rebin to match
  rebinToWorkspace(EXTRACTEDWS + name, ws, EXTRACTEDWS + name);
  // add together
  plus(EXTRACTEDWS + name, ws, EXTRACTEDWS + name);

  // do division
  ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(EXTRACTEDWS + name);
  ws->mutableY(0) /= (double(oldTotalNumber) + 1.0);
  AnalysisDataService::Instance().addOrReplace(EXTRACTEDWS + name, ws);
  m_numberOfTubesInAverage++;
}

/*
 * Transforms ALF data; normalise to current and then converts to d spacing
 * If already d-space does nothing.
 */
void ALFInstrumentModel::transformData() {
  normaliseByCurrent(getWSName(), getWSName());
  convertUnits(getWSName(), "dSpacing", getWSName());
}

void ALFInstrumentModel::extractSingleTube() {
  storeSingleTube(getInstrument() + std::to_string(getCurrentRun()));
  m_numberOfTubesInAverage = 1;
}

void ALFInstrumentModel::storeSingleTube(const std::string &name) {
  auto &ads = AnalysisDataService::Instance();
  if (!ads.doesExist(CURVES))
    return;

  const auto scaleFactor = xConversionFactor(ads.retrieveWS<MatrixWorkspace>(CURVES));
  if (!scaleFactor)
    return;

  // Convert to degrees if the XAxis is an angle in radians
  scaleX(CURVES, *scaleFactor, EXTRACTEDWS + name);

  convertToHistogram(EXTRACTEDWS + name, EXTRACTEDWS + name);

  ads.remove(CURVES);
}

/*
 * Checks loaded data is from ALF
 * Loads data, normalise to current and then converts to d spacing
 * @return pair<bool,bool>:: If the instrument is ALF, if it is d-spacing
 */
std::map<std::string, bool> ALFInstrumentModel::isDataValid() {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getTmpName());
  bool isItALF = false;

  if (ws->getInstrument()->getName() == getInstrument()) {
    isItALF = true;
  }
  auto axis = ws->getAxis(0);
  auto unit = axis->unit()->unitID();
  bool isItDSpace = true;
  if (unit != "dSpacing") {
    isItDSpace = false;
  }
  return {{"IsValidInstrument", isItALF}, {"IsItDSpace", isItDSpace}};
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
    return unit == "Phi" || label == "Out of plane angle" ? 180.0 / M_PI : 1.0;
  }
  return std::nullopt;
}

std::string ALFInstrumentModel::WSName() {
  std::string name = getInstrument() + std::to_string(getCurrentRun());
  return EXTRACTEDWS + name;
}

void ALFInstrumentModel::rename() { AnalysisDataService::Instance().rename(m_tmpName, m_wsName); }
void ALFInstrumentModel::remove() { AnalysisDataService::Instance().remove(m_tmpName); }

std::string ALFInstrumentModel::dataFileName() { return m_wsName; }

int ALFInstrumentModel::currentRun() {
  try {

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
    return ws->getRunNumber();
  } catch (...) {
    return ERRORCODE;
  }
}

bool ALFInstrumentModel::isErrorCode(const int run) const { return (run == ERRORCODE); }

bool ALFInstrumentModel::hasTubeBeenExtracted() const {
  return AnalysisDataService::Instance().doesExist(EXTRACTEDWS + getInstrument() + std::to_string(getCurrentRun()));
}

int ALFInstrumentModel::numberOfTubesInAverage() const { return m_numberOfTubesInAverage; }

} // namespace MantidQt::CustomInterfaces

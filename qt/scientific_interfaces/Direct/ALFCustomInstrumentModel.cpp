// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFCustomInstrumentModel.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace {
const std::string EXTRACTEDWS = "extractedTubes_";
const std::string CURVES = "Curves";
} // namespace

using namespace Mantid::API;
namespace MantidQt {
namespace CustomInterfaces {

ALFCustomInstrumentModel::ALFCustomInstrumentModel() : m_numberOfTubesInAverage(0) {
  m_base = new MantidWidgets::BaseCustomInstrumentModel("ALF_tmp", "ALF", "ALFData");
}

/*
 * Runs load data alg
 * @param name:: string name for ALF data
 */
void ALFCustomInstrumentModel::loadAlg(const std::string &name) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", name);
  alg->setProperty("OutputWorkspace", getTmpName()); // write to tmp ws
  alg->execute();
}

/*
 * Loads data for use in ALFView
 * Loads data, normalise to current and then converts to d spacing
 * @param name:: string name for ALF data
 * @return std::pair<int,std::string>:: the run number and status
 */
std::pair<int, std::string> ALFCustomInstrumentModel::loadData(const std::string &name) {
  loadAlg(name);
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
/*
 * Checks loaded data is from ALF
 * Loads data, normalise to current and then converts to d spacing
 * @return pair<bool,bool>:: If the instrument is ALF, if it is d-spacing
 */
std::map<std::string, bool> ALFCustomInstrumentModel::isDataValid() {
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
 * Transforms ALF data; normalise to current and then converts to d spacing
 * If already d-space does nothing.
 */
void ALFCustomInstrumentModel::transformData() {
  auto normAlg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  normAlg->initialize();
  normAlg->setProperty("InputWorkspace", getWSName());
  normAlg->setProperty("OutputWorkspace", getWSName());
  normAlg->execute();

  auto dSpacingAlg = AlgorithmManager::Instance().create("ConvertUnits");
  dSpacingAlg->initialize();
  dSpacingAlg->setProperty("InputWorkspace", getWSName());
  dSpacingAlg->setProperty("Target", "dSpacing");
  dSpacingAlg->setProperty("OutputWorkspace", getWSName());
  dSpacingAlg->execute();
}

void ALFCustomInstrumentModel::storeSingleTube(const std::string &name) {
  auto alg = AlgorithmManager::Instance().create("ScaleX");
  alg->initialize();
  alg->setProperty("InputWorkspace", CURVES);
  alg->setProperty("OutputWorkspace", EXTRACTEDWS + name);
  alg->setProperty("Factor", 180. / M_PI); // convert to degrees
  alg->execute();

  auto histogramAlg = AlgorithmManager::Instance().create("ConvertToHistogram");
  histogramAlg->initialize();
  histogramAlg->setProperty("InputWorkspace", EXTRACTEDWS + name);
  histogramAlg->setProperty("OutputWorkspace", EXTRACTEDWS + name);
  histogramAlg->execute();

  AnalysisDataService::Instance().remove(CURVES);
}

std::string ALFCustomInstrumentModel::WSName() {
  std::string name = getInstrument() + std::to_string(getCurrentRun());
  return EXTRACTEDWS + name;
}

void ALFCustomInstrumentModel::averageTube() {
  const std::string name = getInstrument() + std::to_string(getCurrentRun());
  const int oldTotalNumber = m_numberOfTubesInAverage;
  // multiply up current average
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(EXTRACTEDWS + name);
  ws *= double(oldTotalNumber);

  // get the data to add
  storeSingleTube(name);
  // rebin to match
  auto rebin = AlgorithmManager::Instance().create("RebinToWorkspace");
  rebin->initialize();
  rebin->setProperty("WorkspaceToRebin", EXTRACTEDWS + name);
  rebin->setProperty("WorkspaceToMatch", ws);
  rebin->setProperty("OutputWorkspace", EXTRACTEDWS + name);
  rebin->execute();

  // add together
  auto alg = AlgorithmManager::Instance().create("Plus");
  alg->initialize();
  alg->setProperty("LHSWorkspace", EXTRACTEDWS + name);
  alg->setProperty("RHSWorkspace", ws);
  alg->setProperty("OutputWorkspace", EXTRACTEDWS + name);
  alg->execute();
  // do division
  ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(EXTRACTEDWS + name);
  ws->mutableY(0) /= (double(oldTotalNumber) + 1.0);
  AnalysisDataService::Instance().addOrReplace(EXTRACTEDWS + name, ws);
  m_numberOfTubesInAverage++;
}

bool ALFCustomInstrumentModel::hasTubeBeenExtracted(const std::string &name) {
  return AnalysisDataService::Instance().doesExist(EXTRACTEDWS + name);
}

bool ALFCustomInstrumentModel::extractTubeCondition(std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStored")->second || tabBools.find("hasCurve")->second);
    return (tabBools.find("isTube")->second && ifCurve);
  } catch (...) {
    return false;
  }
}

bool ALFCustomInstrumentModel::averageTubeCondition(std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStored")->second || tabBools.find("hasCurve")->second);
    return (m_numberOfTubesInAverage > 0 && tabBools.find("isTube")->second && ifCurve &&
            hasTubeBeenExtracted(getInstrument() + std::to_string(getCurrentRun())));
  } catch (...) {
    return false;
  }
}
void ALFCustomInstrumentModel::extractSingleTube() {
  storeSingleTube(getInstrument() + std::to_string(getCurrentRun()));
  m_numberOfTubesInAverage = 1;
}

CompositeFunction_sptr ALFCustomInstrumentModel::getDefaultFunction() {

  CompositeFunction_sptr composite = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
      Mantid::API::FunctionFactory::Instance().createFunction("CompositeFunction"));

  auto func = Mantid::API::FunctionFactory::Instance().createInitialized("name = FlatBackground");
  composite->addFunction(func);
  func = Mantid::API::FunctionFactory::Instance().createInitialized("name = Gaussian, Height = 3., Sigma= 1.0");
  composite->addFunction(func);
  return composite;
}

} // namespace CustomInterfaces
} // namespace MantidQt

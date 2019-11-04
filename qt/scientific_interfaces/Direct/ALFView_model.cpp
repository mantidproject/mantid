// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "ALFView_model.h"
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

ALFView_model::ALFView_model() : m_numberOfTubesInAverage(0) {
  m_tmpName = "ALF_tmp";
  m_instrumentName = "ALF";
  m_wsName = "ALFData";
  m_currentRun = 0;
}

/*
 * Loads data for use in ALFView
 * Loads data, normalise to current and then converts to d spacing
 * @param name:: string name for ALF data
 * @return std::pair<int,std::string>:: the run number and status
 */
std::pair<int, std::string> ALFView_model::loadData(const std::string &name) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", name);
  alg->setProperty("OutputWorkspace", m_tmpName); // write to tmp ws
  alg->execute();
  auto ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_tmpName);
  int runNumber = ws->getRunNumber();
  std::string message = "success";
  auto bools = isDataValid();
  if (bools["IsValidInstrument"]) {
    rename();
    m_numberOfTubesInAverage = 0;
  } else {
    // reset to the previous data
    message = "Not the corrct instrument, expected " + m_instrumentName;
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
std::map<std::string, bool> ALFView_model::isDataValid() {
  auto ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_tmpName);
  bool isItALF = false;
  bool isItDSpace = false;

  if (ws->getInstrument()->getName() == m_instrumentName) {
    isItALF = true;
  }
  auto axis = ws->getAxis(0);
  auto unit = axis->unit()->unitID();
  if (unit == "dSpacing") {
    isItDSpace = true;
  }
  return {{"IsValidInstrument", isItALF}, {"IsItDSpace", isItDSpace}};
}

/*
 * Transforms ALF data; normalise to current and then converts to d spacing
 * If already d-space does nothing.
 */
void ALFView_model::transformData() {
  auto normAlg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  normAlg->initialize();
  normAlg->setProperty("InputWorkspace", m_wsName);
  normAlg->setProperty("OutputWorkspace", m_wsName);
  normAlg->execute();

  auto dSpacingAlg = AlgorithmManager::Instance().create("ConvertUnits");
  dSpacingAlg->initialize();
  dSpacingAlg->setProperty("InputWorkspace", m_wsName);
  dSpacingAlg->setProperty("Target", "dSpacing");
  dSpacingAlg->setProperty("OutputWorkspace", m_wsName);
  dSpacingAlg->execute();
}

void ALFView_model::storeSingleTube(const std::string &name) {
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

std::string ALFView_model::WSName() {
  std::string name = m_instrumentName + std::to_string(getCurrentRun());
  return EXTRACTEDWS + name;
}

void ALFView_model::averageTube() {
  const std::string name = m_instrumentName + std::to_string(getCurrentRun());
  const int oldTotalNumber = m_numberOfTubesInAverage;
  // multiply up current average
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      EXTRACTEDWS + name);
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
  ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(EXTRACTEDWS +
                                                                   name);
  ws->mutableY(0) /= (double(oldTotalNumber) + 1.0);
  AnalysisDataService::Instance().addOrReplace(EXTRACTEDWS + name, ws);
  m_numberOfTubesInAverage++;
}

bool ALFView_model::hasTubeBeenExtracted(const std::string &name) {
  return AnalysisDataService::Instance().doesExist(EXTRACTEDWS + name);
}

bool ALFView_model::extractTubeConditon(std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStored")->second ||
                    tabBools.find("hasCurve")->second);
    return (tabBools.find("isTube")->second && ifCurve);
  } catch (...) {
    return false;
  }
}

bool ALFView_model::averageTubeConditon(std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStored")->second ||
                    tabBools.find("hasCurve")->second);
    return (m_numberOfTubesInAverage > 0 && tabBools.find("isTube")->second &&
            ifCurve &&
            hasTubeBeenExtracted(m_instrumentName +
                                 std::to_string(getCurrentRun())));
  } catch (...) {
    return false;
  }
}
void ALFView_model::extractSingleTube() {
  storeSingleTube(m_instrumentName + std::to_string(getCurrentRun()));
  m_numberOfTubesInAverage = 1;
}

CompositeFunction_sptr ALFView_model::getDefaultFunction() {

  CompositeFunction_sptr composite =
      boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
          Mantid::API::FunctionFactory::Instance().createFunction(
              "CompositeFunction"));

  auto func = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = FlatBackground");
  composite->addFunction(func);
  func = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = Gaussian, Height = 3., Sigma= 1.0");
  composite->addFunction(func);
  return composite;
}

} // namespace CustomInterfaces
} // namespace MantidQt

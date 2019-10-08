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
const std::string TMPNAME = "ALF_tmp";
const std::string INSTRUMENTNAME = "ALF";
const std::string WSNAME = "ALFData";
const int ERRORCODE = -999;
const std::string EXTRACTEDWS = "extractedTubes_";
const std::string CURVES = "Curves";
} // namespace

using namespace Mantid::API;
namespace MantidQt {
namespace CustomInterfaces {

void ALFView_model::loadEmptyInstrument() {
  auto alg =
      Mantid::API::AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("OutputWorkspace", WSNAME);
  alg->setProperty("InstrumentName", INSTRUMENTNAME);
  alg->execute();
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
  alg->setProperty("OutputWorkspace", TMPNAME); // write to tmp ws
  alg->execute();
  auto ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TMPNAME);
  int runNumber = ws->getRunNumber();
  std::string message = "success";
  auto bools = isDataValid();
  if (bools["IsValidInstrument"]) {
    rename();

  } else {
    // reset to the previous data
    message = "Not the corrct instrument, expected " + INSTRUMENTNAME;
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
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TMPNAME);
  bool isItALF = false;
  bool isItDSpace = false;

  if (ws->getInstrument()->getName() == INSTRUMENTNAME) {
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
  normAlg->setProperty("InputWorkspace", WSNAME);
  normAlg->setProperty("OutputWorkspace", WSNAME);
  normAlg->execute();

  auto dSpacingAlg = AlgorithmManager::Instance().create("ConvertUnits");
  dSpacingAlg->initialize();
  dSpacingAlg->setProperty("InputWorkspace", WSNAME);
  dSpacingAlg->setProperty("Target", "dSpacing");
  dSpacingAlg->setProperty("OutputWorkspace", WSNAME);
  dSpacingAlg->execute();
}

void ALFView_model::rename() {
  AnalysisDataService::Instance().rename(TMPNAME, WSNAME);
}
void ALFView_model::remove() {
  AnalysisDataService::Instance().remove(TMPNAME);
}

std::string ALFView_model::dataFileName() { return WSNAME; }

int ALFView_model::currentRun() {
  try {

    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(WSNAME);
    return ws->getRunNumber();
  } catch (...) {
    return ERRORCODE;
  }
}

bool ALFView_model::isErrorCode(const int run) { return (run == ERRORCODE); }

std::string ALFView_model::getInstrument() { return INSTRUMENTNAME; }

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

void ALFView_model::averageTube(const int &oldTotalNumber,
                                const std::string &name) {
  // multiply up current average
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      EXTRACTEDWS + name);
  ws->mutableY(0) * double(oldTotalNumber);

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
}

bool ALFView_model::hasTubeBeenExtracted(const std::string &name) {
  return AnalysisDataService::Instance().doesExist(EXTRACTEDWS + name);
}

} // namespace CustomInterfaces
} // namespace MantidQt

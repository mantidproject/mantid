#include "ALFView_model.h"
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidGeometry/Instrument.h"

#include "MantidKernel/Unit.h"

#include <utility>

namespace {
const std::string tmpName = "ALF_tmp";
const std::string instrumentName = "ALF";
const std::string wsName = "ALFData";
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace Direct {

void loadEmptyInstrument() {
  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("OutputWorkspace", wsName);
  alg->setProperty("InstrumentName", instrumentName);
  alg->execute();
}
/*
 * Loads data for use in ALFView
 * Loads data, normalise to current and then converts to d spacing
 * @param name:: string name for ALF data
 * @return int:: the run number
 */
int loadData(const std::string &name) {
  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", name);
  alg->setProperty("OutputWorkspace", tmpName); // write to tmp ws
  alg->execute();
  Mantid::API::MatrixWorkspace_sptr ws =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::MatrixWorkspace>(tmpName);
  return ws->getRunNumber();
}
/*
 * Checks loaded data is from ALF
 * Loads data, normalise to current and then converts to d spacing
 * @return pair<bool,bool>:: If the instrument is ALF, if it is d-spacing
 */
std::pair<bool, bool> isDataValid() {
  Mantid::API::MatrixWorkspace_sptr ws =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::MatrixWorkspace>(tmpName);

  bool isItALF = false;
  bool isItDSpace = false;

  if (ws->getInstrument()->getName() == instrumentName) {
    isItALF = true;
  }
  auto axis = ws->getAxis(0);
  auto unit = axis->unit()->unitID();
  if (unit == "dSpacing") {
    isItDSpace = true;
  }

return std::make_pair(isItALF, isItDSpace);
}

/*
 * Transforms ALF data; normalise to current and then converts to d spacing
 * If already d-space does nothing.
 */
void transformData() {
  Mantid::API::IAlgorithm_sptr normAlg =
      Mantid::API::AlgorithmManager::Instance().create("NormaliseByCurrent");
  normAlg->initialize();
  normAlg->setProperty("InputWorkspace", wsName);
  normAlg->setProperty("OutputWorkspace", wsName);
  normAlg->execute();

  Mantid::API::IAlgorithm_sptr dSpacingAlg =
      Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
  dSpacingAlg->initialize();
  dSpacingAlg->setProperty("InputWorkspace", wsName);
  dSpacingAlg->setProperty("Target", "dSpacing");
  dSpacingAlg->setProperty("OutputWorkspace", wsName);
  dSpacingAlg->execute();
}

void rename() {
  Mantid::API::AnalysisDataService::Instance().rename(tmpName, wsName);
}
void remove() { Mantid::API::AnalysisDataService::Instance().remove(tmpName); }

int currentRun() {
  try {

    Mantid::API::MatrixWorkspace_sptr ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(wsName);
    return ws->getRunNumber();
  } catch(...) {
    return -999; // special error code
  }
}

} // namespace Direct
} // namespace CustomInterfaces
} // namespace MantidQt

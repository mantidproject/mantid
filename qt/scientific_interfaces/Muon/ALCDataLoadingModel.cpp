// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ALCDataLoadingModel.h"
#include "IALCDataLoadingView.h"

#include "MantidAPI/AlgorithmManager.h"
// #include "MantidAPI/Run.h"
// #include "MantidGeometry/Instrument.h"
// #include "MantidKernel/InstrumentInfo.h"
// #include "MantidKernel/Strings.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

ALCDataLoadingModel::ALCDataLoadingModel()
    : m_numDetectors(0), m_loadingData(false), m_directoryChanged(false), m_lastRunLoadedAuto(-2), m_filesLoaded(),
      m_wasLastAutoRange(false), m_previousFirstRun("") {}

void ALCDataLoadingModel::setLoadingData(bool isLoading) { m_loadingData = isLoading; }
/**
 * Load new data and update the view accordingly
 * @param files :: [input] range of files (user-specified or auto generated)
 */
void ALCDataLoadingModel::load(const std::vector<std::string> &files, const IALCDataLoadingView *view) {

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
  alg->setAlwaysStoreInADS(false); // Don't want workspaces in the ADS

  // Change first last run to WorkspaceNames
  alg->setProperty("WorkspaceNames", files);
  alg->setProperty("LogValue", view->log());
  alg->setProperty("Function", view->function());
  alg->setProperty("Type", view->calculationType());
  alg->setProperty("DeadTimeCorrType", view->deadTimeType());
  alg->setProperty("Red", view->redPeriod());

  // If time limiting requested, set min/max times
  if (auto timeRange = view->timeRange()) {
    double timeMin = (*timeRange).first;
    double timeMax = (*timeRange).second;
    if (timeMin >= timeMax) {
      throw std::invalid_argument("Invalid time limits");
    }
    alg->setProperty("TimeMin", timeMin);
    alg->setProperty("TimeMax", timeMax);
  }

  // If corrections from custom file requested, set file property
  if (view->deadTimeType() == "FromSpecifiedFile") {
    alg->setProperty("DeadTimeCorrFile", view->deadTimeFile());
  }

  // If custom grouping requested, set forward/backward groupings
  if (view->detectorGroupingType() == "Custom") {
    alg->setProperty("ForwardSpectra", view->getForwardGrouping());
    alg->setProperty("BackwardSpectra", view->getBackwardGrouping());
  }

  // Set alpha for balance parameter
  alg->setProperty("Alpha", view->getAlphaValue());

  // If Subtract checkbox is selected, set green period
  if (view->subtractIsChecked()) {
    alg->setProperty("Green", view->greenPeriod());
  }

  alg->setPropertyValue("OutputWorkspace", "__NotUsed");

  // Set loading alg equal to alg
  this->m_LoadingAlg = alg;
  // Execute async so we can show progress bar
  Poco::ActiveResult<bool> result(alg->executeAsync());
  while (!result.available()) {
    QCoreApplication::processEvents();
  }
  if (!result.error().empty()) {
    throw std::runtime_error(result.error());
  }

  MatrixWorkspace_sptr tmp = alg->getProperty("OutputWorkspace");
  IAlgorithm_sptr sortAlg = AlgorithmManager::Instance().create("SortXAxis");
  sortAlg->setAlwaysStoreInADS(false); // Don't want workspaces in the ADS
  sortAlg->setProperty("InputWorkspace", tmp);
  sortAlg->setProperty("Ordering", "Ascending");
  sortAlg->setProperty("OutputWorkspace", "__NotUsed__");
  sortAlg->execute();

  m_loadedData = sortAlg->getProperty("OutputWorkspace");

  // If errors are properly caught, shouldn't happen
  assert(m_loadedData);
  // If subtract is not checked, only one spectrum,
  // else four spectra
  if (!view->subtractIsChecked()) {
    assert(m_loadedData->getNumberHistograms() == 1);
  } else {
    assert(m_loadedData->getNumberHistograms() == 4);
  }
}

} // namespace MantidQt::CustomInterfaces

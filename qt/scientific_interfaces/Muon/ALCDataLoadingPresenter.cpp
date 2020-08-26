// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"

#include "ALCLatestFileFinder.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MuonAnalysisHelper.h"

#include "Poco/File.h"
#include <Poco/ActiveResult.h>
#include <Poco/Path.h>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;

namespace MantidQt {
namespace CustomInterfaces {
ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView *view)
    : m_view(view), m_numDetectors(0), m_loadingData(false) {}

void ALCDataLoadingPresenter::initialize() {
  m_view->initialize();

  connect(m_view, SIGNAL(loadRequested()), SLOT(handleLoadRequested()));
  connect(m_view, SIGNAL(runsSelected()), SLOT(updateAvailableInfo()));
  connect(m_view, SIGNAL(runAutoChecked()), SLOT(updateAutoRun()));
  connect(m_view, SIGNAL(runAutoUnchecked()), SLOT(resetAutoRun()));
}

/**
 * Called when the Load button is clicked.
 * Gets last run, passes it to load method if not "auto".
 * If it was "auto", sets up a watcher to automatically reload on new files.
 */
void ALCDataLoadingPresenter::handleLoadRequested() {
  std::vector<std::string> files(m_view->getRuns());

  // Check not empty
  if (files.empty()) {
    m_view->displayError("Not a valid expression for runs");
    return;
  }

  // Now perform the load
  load(files);
}


void ALCDataLoadingPresenter::updateRunsTextFromAuto(const int autoRun) {

  const int currentLastRun = m_view->extractRunNumber(m_view->lastRun());
  //auto currentInput = m_ui.runs->getText().toStdString();
  auto currentInput = m_view->getCurrentRunsText();

  // Only make changes if found run > user last run
  if (autoRun < currentLastRun)
    return;

  // Save old input
  //m_oldInput = currentInput;
  m_view->setRunsOldInput(currentInput);

  // Check if range at end
  std::size_t findRange = currentInput.find_last_of("-");
  std::size_t findComma = currentInput.find_last_of(",");
  QString newInput;

  // Remove ending range if at end of input
  if (findRange != -1 && (findComma == -1 || findRange > findComma)) {
    currentInput.erase(findRange, currentInput.length() - 1);
  }

  // Initialise new input
  newInput = QString::fromStdString(currentInput);

  // Will hold the base path for all runs, used to check which run numbers
  // exist
  auto basePath = m_view->firstRun();

  // Strip the extension
  size_t findExt = basePath.find_last_of(".");
  const auto ext = basePath.substr(findExt);
  basePath.erase(findExt);

  // Remove the run number part so we are left with just the base path
  const std::string numPart = std::to_string(currentLastRun);
  basePath.erase(basePath.length() - numPart.length());

  bool fnf = false; // file not found

  // Check all files valid between current last and auto, remove bad ones
  for (int i = currentLastRun + 1; i < autoRun; ++i) {

    // Try creating a file from base, i and extension
    Poco::File testFile(basePath + std::to_string(i) + ext);

    // If doesn't exist add range to previous run
    if (testFile.exists()) {

      if (fnf) { // Means next file found since a file not found
        // Add comma
        newInput.append(",");
        newInput.append(QString::number(i));
        fnf = false;
      }
    } else {
      // Edge case do not add range
      if (i-1 != currentLastRun && i+1 != autoRun) {
        newInput.append("-");
        newInput.append(QString::number(i - 1));
        fnf = true;
      } else {
        fnf = true;
      }
    }
  }

  // If true then need a comma instead as file before last is missing
  if (fnf)
    newInput.append(",");
  else
    newInput.append("-");
  newInput.append(QString::number(autoRun));

  // Update it
  //m_ui.runs->setFileTextWithSearch(newInput);
  m_view->setRunsTextWithSearch(newInput);
}


/**
 * Called when the auto checkbox is checked
 * Tries to find the most recent file in the same directory as first
 * and save the run number
 */
void ALCDataLoadingPresenter::updateAutoRun() {

  // Find most recent file from first
  ALCLatestFileFinder finder(m_view->firstRun());
  const auto last = finder.getMostRecentFile();

  // If empty auto cannot be used
  if (last.empty()) {
    m_view->displayError("Could not determine a valid last file.");
    m_view->setCurrentAutoRun(-1);
    return;
  }

  // Update the auto run in the view
  const int lastRun = m_view->extractRunNumber(last);
  m_view->setCurrentAutoRun(lastRun);

  // Update text
  updateRunsTextFromAuto(lastRun);
}

void ALCDataLoadingPresenter::resetAutoRun() {
  m_view->setRunsTextWithSearch(QString::fromStdString(m_view->getRunsOldInput()));
}

/**
 * Load new data and update the view accordingly
 * @param lastFile :: [input] Last file in range (user-specified or auto)
 */
void ALCDataLoadingPresenter::load(const std::vector<std::string> &files) {

  m_loadingData = true;
  m_view->disableAll();

  // Before loading, check custom grouping (if used) is sensible
  const bool groupingOK = checkCustomGrouping();
  if (!groupingOK) {
    m_view->displayError(
        "Custom grouping not valid (bad format or detector numbers)");
    m_view->enableAll();
    return;
  }

  if (files.empty()) {
    m_view->displayError("The list of files to load is empty. ");
    m_view->enableAll();
    m_loadingData = false;
    return;
  }

  try {

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
    alg->setChild(true); // Don't want workspaces in the ADS

    // Change first last run to WorkspaceNames
    alg->setProperty("WorkspaceNames", files);
    alg->setProperty("LogValue", m_view->log());
    alg->setProperty("Function", m_view->function());
    alg->setProperty("Type", m_view->calculationType());
    alg->setProperty("DeadTimeCorrType", m_view->deadTimeType());
    alg->setProperty("Red", m_view->redPeriod());

    // If time limiting requested, set min/max times
    if (auto timeRange = m_view->timeRange()) {
      double timeMin = (*timeRange).first;
      double timeMax = (*timeRange).second;
      if (timeMin >= timeMax) {
        throw std::invalid_argument("Invalid time limits");
      }
      alg->setProperty("TimeMin", timeMin);
      alg->setProperty("TimeMax", timeMax);
    }

    // If corrections from custom file requested, set file property
    if (m_view->deadTimeType() == "FromSpecifiedFile") {
      alg->setProperty("DeadTimeCorrFile", m_view->deadTimeFile());
    }

    // If custom grouping requested, set forward/backward groupings
    if (m_view->detectorGroupingType() == "Custom") {
      alg->setProperty("ForwardSpectra", m_view->getForwardGrouping());
      alg->setProperty("BackwardSpectra", m_view->getBackwardGrouping());
    }

    // If Subtract checkbox is selected, set green period
    if (m_view->subtractIsChecked()) {
      alg->setProperty("Green", m_view->greenPeriod());
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
    sortAlg->setChild(true); // Don't want workspaces in the ADS
    sortAlg->setProperty("InputWorkspace", tmp);
    sortAlg->setProperty("Ordering", "Ascending");
    sortAlg->setProperty("OutputWorkspace", "__NotUsed__");
    sortAlg->execute();

    m_loadedData = sortAlg->getProperty("OutputWorkspace");

    // If errors are properly caught, shouldn't happen
    assert(m_loadedData);
    // If subtract is not checked, only one spectrum,
    // else four spectra
    if (!m_view->subtractIsChecked()) {
      assert(m_loadedData->getNumberHistograms() == 1);
    } else {
      assert(m_loadedData->getNumberHistograms() == 4);
    }

    const auto xvalOne = m_loadedData->readX(0)[0];
    const auto xvalTwo = m_loadedData->readX(0)[1];
    const auto xvalThree = m_loadedData->readX(0)[2];

    const auto yvalOne = m_loadedData->readY(0)[0];
    const auto yvalTwo = m_loadedData->readY(0)[1];
    const auto yvalThree = m_loadedData->readY(0)[2];

    // Plot spectrum 0. It is either red period (if subtract is unchecked) or
    // red - green (if subtract is checked)
    m_view->setDataCurve(m_loadedData);

    emit dataChanged();

  } catch (const std::invalid_argument &e) {
    m_view->displayError(e.what());
  } catch (std::exception &e) {
    m_view->displayError(e.what());
  }
  m_view->enableAll();
  m_loadingData = false;
}

void ALCDataLoadingPresenter::updateAvailableInfo() {
  Workspace_sptr loadedWs;
  double firstGoodData = 0, timeZero = 0;

  try //... to load the first run
  {
    IAlgorithm_sptr loadAlg =
        AlgorithmManager::Instance().create("LoadMuonNexus");
    loadAlg->setChild(true); // Don't want workspaces in the ADS
    loadAlg->setProperty("Filename", m_view->firstRun());
    // We need logs only but we have to use LoadMuonNexus
    // (can't use LoadMuonLogs as not all the logs would be
    // loaded), so we load the minimum amount of data, i.e., one spectrum
    loadAlg->setPropertyValue("SpectrumMin", "1");
    loadAlg->setPropertyValue("SpectrumMax", "1");
    loadAlg->setPropertyValue("OutputWorkspace", "__NotUsed");
    loadAlg->execute();

    loadedWs = loadAlg->getProperty("OutputWorkspace");
    firstGoodData = loadAlg->getProperty("FirstGoodData");
    timeZero = loadAlg->getProperty("TimeZero");
  } catch (...) {
    m_view->setAvailableLogs(std::vector<std::string>()); // Empty logs list
    m_view->setAvailablePeriods(
        std::vector<std::string>()); // Empty period list
    m_view->setTimeLimits(0, 0);     // "Empty" time limits
    return;
  }

  // Set logs
  MatrixWorkspace_const_sptr ws = MuonAnalysisHelper::firstPeriod(loadedWs);
  std::vector<std::string> logs;

  const auto &properties = ws->run().getProperties();
  for (auto property : properties) {
    logs.emplace_back(property->name());
  }
  m_view->setAvailableLogs(logs);

  // Set periods
  size_t numPeriods = MuonAnalysisHelper::numPeriods(loadedWs);
  std::vector<std::string> periods;
  for (size_t i = 0; i < numPeriods; i++) {
    std::stringstream buffer;
    buffer << i + 1;
    periods.emplace_back(buffer.str());
  }
  m_view->setAvailablePeriods(periods);

  // Set time limits if this is the first data loaded (will both be zero)
  if (auto timeLimits = m_view->timeRange()) {
    if (std::abs(timeLimits->first) < 0.0001 &&
        std::abs(timeLimits->second) < 0.0001) {
      m_view->setTimeLimits(firstGoodData - timeZero, ws->x(0).back());
    }
  }

  // Update number of detectors for this new first run
  m_numDetectors = ws->getInstrument()->getNumberDetectors();
}

MatrixWorkspace_sptr ALCDataLoadingPresenter::exportWorkspace() {
  if (m_loadedData)
    return std::const_pointer_cast<MatrixWorkspace>(m_loadedData);
  return MatrixWorkspace_sptr();
}

void ALCDataLoadingPresenter::setData(const MatrixWorkspace_sptr &data) {

  if (data) {
    // Set the data
    m_loadedData = data;
    // Plot the data
    m_view->setDataCurve(m_loadedData);

  } else {
    std::invalid_argument("Cannot load an empty workspace");
  }
}

/**
 * If custom grouping is supplied, check all detector numbers are valid
 * @returns :: True if grouping OK, false if bad
 */
bool ALCDataLoadingPresenter::checkCustomGrouping() {
  bool groupingOK = true;
  if (m_view->detectorGroupingType() == "Custom") {
    auto detectors = Mantid::Kernel::Strings::parseRange(
        isCustomGroupingValid(m_view->getForwardGrouping(), groupingOK));
    const auto backward = Mantid::Kernel::Strings::parseRange(
        isCustomGroupingValid(m_view->getBackwardGrouping(), groupingOK));
    if (!groupingOK) {
      return false;
    }
    detectors.insert(detectors.end(), backward.begin(), backward.end());
    if (std::any_of(detectors.cbegin(), detectors.cend(),
                    [this](const auto det) {
                      return det < 0 || det > static_cast<int>(m_numDetectors);
                    })) {
      groupingOK = false;
    }
  }
  return groupingOK;
}
/**
 * Check basic group string is valid
 * i.e. does not contain letters or start with , or -
 * @param group :: the string of the grouping
 * @param isValid :: bool to say if the string is valid
 * @returns :: True if grouping OK, false if bad
 */
std::string
ALCDataLoadingPresenter::isCustomGroupingValid(const std::string &group,
                                               bool &isValid) {
  if (!std::isdigit(group[0]) ||
      std::any_of(std::begin(group), std::end(group), ::isalpha)) {
    isValid = false;
    return "";
  }
  return group;
}
/**
 * If currently loading data
 * @returns :: True if currently in load() method
 */
bool ALCDataLoadingPresenter::isLoading() const { return m_loadingData; }
/**
 * Cancels current loading algorithm
 */
void ALCDataLoadingPresenter::cancelLoading() const { m_LoadingAlg->cancel(); }
} // namespace CustomInterfaces
} // namespace MantidQt

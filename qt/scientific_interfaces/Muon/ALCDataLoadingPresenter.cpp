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
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Strings.h"

#include "ALCLatestFileFinder.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MuonAnalysisHelper.h"

#include "Poco/File.h"
#include <Poco/ActiveResult.h>
#include <Poco/Path.h>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <sstream>

namespace {
const int RUNS_WARNING_LIMIT = 200;
// must include the "."
const std::vector<std::string> ADDITIONAL_EXTENSIONS{".nxs", ".nxs_v2", ".bin"};

bool is_decimal(const char character) { return character == '.'; }
} // namespace

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt::CustomInterfaces {
ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView *view)
    : m_view(view), m_periodInfo(std::make_unique<MuonPeriodInfo>()), m_numDetectors(0), m_loadingData(false),
      m_directoryChanged(false), m_timerID(), m_lastRunLoadedAuto(-2), m_filesLoaded(), m_wasLastAutoRange(false),
      m_previousFirstRun("") {}

void ALCDataLoadingPresenter::initialize() {
  m_view->initialize();

  connect(m_view, SIGNAL(loadRequested()), SLOT(handleLoadRequested()));
  connect(m_view, SIGNAL(instrumentChangedSignal(std::string)), SLOT(handleInstrumentChanged(std::string)));
  connect(m_view, SIGNAL(runsEditingSignal()), SLOT(handleRunsEditing()));
  connect(m_view, SIGNAL(runsEditingFinishedSignal()), SLOT(handleRunsEditingFinished()));
  connect(m_view, SIGNAL(manageDirectoriesClicked()), SLOT(handleManageDirectories()));
  connect(m_view, SIGNAL(runsFoundSignal()), SLOT(handleRunsFound()));
  connect(m_view, SIGNAL(autoAddToggledSignal(bool)), SLOT(startWatching(bool)));
  connect(m_view, SIGNAL(periodInfoClicked()), SLOT(handlePeriodInfoClicked()));
  connect(&m_watcher, SIGNAL(directoryChanged(const QString &)), SLOT(updateDirectoryChangedFlag(const QString &)));

  m_view->setFileExtensions(ADDITIONAL_EXTENSIONS);
}

void ALCDataLoadingPresenter::handleRunsEditing() {
  m_view->enableLoad(false);
  m_view->setPath(std::string{});
}

void ALCDataLoadingPresenter::handleRunsEditingFinished() {
  // Make sure everything is reset
  m_view->enableRunsAutoAdd(false);

  m_view->setLoadStatus("Finding " + m_view->getInstrument() + m_view->getRunsText(), "orange");
  m_view->enableAlpha(false);
  m_view->setAlphaValue("");
  m_view->showAlphaMessage(false);
}

void ALCDataLoadingPresenter::handleRunsFound() {
  // Do a quick check for an empty input, do nothing in this case
  if (m_view->getRunsText().empty()) {
    m_view->setLoadStatus("Waiting", "orange");
    return;
  }

  // Check for errors
  if (!m_view->getRunsError().empty()) {
    m_view->setLoadStatus("Error", "red");
    m_view->displayError(m_view->getRunsError());
    return;
  }

  // Try update info and enable load
  try {
    updateAvailableInfo();
    m_view->enableLoad(true);
    m_view->setLoadStatus("Successfully found " + m_view->getInstrument() + m_view->getRunsText(), "green");
    m_previousFirstRun = m_view->getInstrument() + m_view->getRunsFirstRunText();
  } catch (const std::runtime_error &errorUpdateInfo) {
    m_view->setLoadStatus("Error", "red");
    m_view->displayError(errorUpdateInfo.what());
    m_periodInfo->clear();
  }
}

/**
 * Called when the Load button is clicked.
 * Displays a warning if trying to load over RUNS_WARNING_LIMIT files
 * Passes files to load
 */
void ALCDataLoadingPresenter::handleLoadRequested() {
  auto files = m_view->getFiles();

  // Check there are files
  if (files.empty()) {
    m_view->setLoadStatus("Error", "red");
    m_view->displayError("The list of files to load is empty");
    m_view->enableRunsAutoAdd(false);
    return;
  }

  // Warning message if trying to load excessive number of files
  if (files.size() > RUNS_WARNING_LIMIT) {
    auto continueLoad = m_view->displayWarning("You are attempting to load " + std::to_string(files.size()) +
                                               " runs, are you sure you want to do this?");
    if (!continueLoad)
      return;
  }

  m_view->setLoadStatus("Loading " + m_view->getInstrument() + m_view->getRunsText(), "orange");
  try {
    load(files);
    m_filesLoaded = files;
    m_view->setLoadStatus("Successfully loaded " + m_view->getInstrument() + m_view->getRunsText(), "green");
    m_view->enableRunsAutoAdd(true);

    // If alpha empty, default used is 1 so update interface
    if (m_view->getAlphaValue() == "1.0" && m_view->isAlphaEnabled())
      m_view->setAlphaValue("1.0");
  } catch (const std::runtime_error &errorLoadFiles) {
    m_view->setLoadStatus("Error", "red");
    m_view->displayError(errorLoadFiles.what());
    m_view->enableRunsAutoAdd(false);
    m_view->enableAll();
    m_loadingData = false;
  }
}

/**
 * Remove the run number from a full file path
 * @param file :: [input] full path which contains a run number
 * @return An integer representation of the run number
 */
int ALCDataLoadingPresenter::extractRunNumber(const std::string &file) {
  if (file.empty())
    return -1;

  auto returnVal = file;
  // Strip beginning of path to just the run (e.g. MUSR00015189.nxs)
  std::size_t found = returnVal.find_last_of("/\\");
  returnVal = returnVal.substr(found + 1);

  // Remove all non-digits
  returnVal.erase(std::remove_if(returnVal.begin(), returnVal.end(), [](unsigned char c) { return !std::isdigit(c); }),
                  returnVal.end());

  // Return run number as int (removes leading 0's)
  return std::stoi(returnVal);
}

/**
 * Load new data and update the view accordingly
 * @param files :: [input] range of files (user-specified or auto generated)
 */
void ALCDataLoadingPresenter::load(const std::vector<std::string> &files) {
  m_loadingData = true;
  m_view->disableAll();

  // Before loading, check custom grouping (if used) is sensible
  const bool groupingOK = checkCustomGrouping();
  if (!groupingOK) {
    throw std::runtime_error("Custom grouping not valid (bad format or detector numbers)");
  }

  try {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
    alg->setAlwaysStoreInADS(false); // Don't want workspaces in the ADS

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

    // Set alpha for balance parameter
    alg->setProperty("Alpha", m_view->getAlphaValue());

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
    if (!m_view->subtractIsChecked()) {
      assert(m_loadedData->getNumberHistograms() == 1);
    } else {
      assert(m_loadedData->getNumberHistograms() == 4);
    }
    // Plot spectrum 0. It is either red period (if subtract is unchecked) or
    // red - green (if subtract is checked)
    m_view->setDataCurve(m_loadedData);

    emit dataChanged();

  } catch (const std::invalid_argument &e) {
    throw std::runtime_error(e.what()); // Caught in handle load request
  } catch (std::exception &e) {
    throw std::runtime_error(e.what()); // Caught in handle load request
  }
  m_view->enableAll();
  m_loadingData = false;
}

void ALCDataLoadingPresenter::updateAvailableInfo() {
  Workspace_sptr loadedWs;
  double firstGoodData = 0, timeZero = 0;

  try //... to load the first run
  {
    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->setChild(true); // Don't want workspaces in the ADS

    // We need logs only but we have to use Load
    // (can't use LoadMuonLogs as not all the logs would be
    // loaded), so we load the minimum amount of data, i.e., one spectrum
    loadAlg->setProperty("Filename", m_view->getFirstFile());
    loadAlg->setPropertyValue("SpectrumMin", "1");
    loadAlg->setPropertyValue("SpectrumMax", "1");
    loadAlg->setPropertyValue("OutputWorkspace", "__NotUsed");
    loadAlg->execute();

    loadedWs = loadAlg->getProperty("OutputWorkspace");
    firstGoodData = loadAlg->getProperty("FirstGoodData");
    timeZero = loadAlg->getProperty("TimeZero");
  }
  catch (const std::exception &error) {
    m_view->setAvailableInfoToEmpty();
    throw std::runtime_error(error.what());
  }

  // Set path
  m_view->setPath(getPathFromFiles());

  // Set logs
  const MatrixWorkspace_sptr ws = MuonAnalysisHelper::firstPeriod(loadedWs);
  std::vector<std::string> logs;

  const auto &properties = ws->run().getProperties();
  std::transform(properties.cbegin(), properties.cend(), std::back_inserter(logs),
                 [](const auto &property) { return property->name(); });

  // sort alphabetically
  // cannot use standard sort alone as some logs are capitalised and some are
  // not
  std::sort(logs.begin(), logs.end(), [](const auto &log1, const auto &log2) {
    // compare logs char by char and return pair of non-equal elements
    const auto result =
        std::mismatch(log1.cbegin(), log1.cend(), log2.cbegin(), log2.cend(),
                      [](const auto &lhs, const auto &rhs) { return std::tolower(lhs) == std::tolower(rhs); });
    // compare the two elements to decide which log should go first
    return result.second != log2.cend() &&
           (result.first == log1.cend() || std::tolower(*result.first) < std::tolower(*result.second));
  });

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

  // If single period, enable alpha, otherwise disable
  if (numPeriods == 1) {
    m_view->enableAlpha(true);
    m_view->setAlphaValue("1.0");
    m_view->showAlphaMessage(false);
  } else {
    m_view->enableAlpha(false);
    m_view->showAlphaMessage(true);
  }

  // Update available period info
  updateAvailablePeriodInfo(ws);

  // Set time limits if this is the first data loaded (will both be zero)
  if (auto timeLimits = m_view->timeRange()) {
    if (std::abs(timeLimits->first) < 0.0001 && std::abs(timeLimits->second) < 0.0001) {
      m_view->setTimeLimits(firstGoodData - timeZero, ws->x(0).back());
    }
  }

  // Update number of detectors for this new first run
  m_numDetectors = ws->getInstrument()->getNumberDetectors();
}

std::string ALCDataLoadingPresenter::getPathFromFiles() const {
  const auto files = m_view->getFiles();
  if (files.empty())
    return "";
  const auto firstDirectory = files[0u].substr(0u, files[0u].find_last_of("/\\"));
  // Lambda to compare directories from a path
  const auto hasSameDirectory = [&firstDirectory](const auto &path) {
    return path.substr(0u, path.find_last_of("/\\")) == firstDirectory;
  };
  const auto sameDirectory = std::all_of(files.cbegin(), files.cend(), hasSameDirectory);
  return sameDirectory ? firstDirectory : "Multiple Directories";
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
    throw std::invalid_argument("Cannot load an empty workspace");
  }
}

/**
 * If custom grouping is supplied, check all detector numbers are valid
 * @returns :: True if grouping OK, false if bad
 */
bool ALCDataLoadingPresenter::checkCustomGrouping() {
  bool groupingOK = true;
  if (m_view->detectorGroupingType() == "Custom") {
    auto detectors =
        Mantid::Kernel::Strings::parseRange(isCustomGroupingValid(m_view->getForwardGrouping(), groupingOK));
    const auto backward =
        Mantid::Kernel::Strings::parseRange(isCustomGroupingValid(m_view->getBackwardGrouping(), groupingOK));
    if (!groupingOK) {
      return false;
    }
    detectors.insert(detectors.end(), backward.begin(), backward.end());
    if (std::any_of(detectors.cbegin(), detectors.cend(),
                    [this](const auto det) { return det < 0 || det > static_cast<int>(m_numDetectors); })) {
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
std::string ALCDataLoadingPresenter::isCustomGroupingValid(const std::string &group, bool &isValid) {
  if (!std::isdigit(static_cast<unsigned char>(group[0])) ||
      std::any_of(std::begin(group), std::end(group), ::isalpha) ||
      std::any_of(std::begin(group), std::end(group), is_decimal)) {
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

void ALCDataLoadingPresenter::handleInstrumentChanged(const std::string &instrument) {
  // Clear path as instrument has changed
  m_view->setPath(std::string{});

  // Update instrument
  m_view->setInstrument(instrument);

  // User cannot load yet as path not set
  m_view->enableLoad(false);

  // Turn off auto add
  m_view->enableRunsAutoAdd(false);
  m_view->toggleRunsAutoAdd(false);
}

void ALCDataLoadingPresenter::handleManageDirectories() {
  MantidQt::API::ManageUserDirectories::openManageUserDirectories();
}

/**
 * The watched directory has been changed - update flag.
 * @param path :: [input] Path to directory modified (not used)
 */
void ALCDataLoadingPresenter::updateDirectoryChangedFlag(const QString &path) {
  Q_UNUSED(path); // just set the flag, don't need the path
  m_directoryChanged = true;
}

void ALCDataLoadingPresenter::startWatching(bool watch) {
  if (watch) {
    // Get path to watch and add to watcher
    const auto path = m_view->getPath();
    m_watcher.addPath(QString::fromStdString(path));

    // start a timer that executes every second
    m_timerID = startTimer(1000);
  } else {
    // Check if watcher has a directory, then remove all
    if (!m_watcher.directories().empty()) {
      m_watcher.removePaths(m_watcher.directories());
    }

    // Stop timer
    killTimer(m_timerID);

    // Reset latest auto run number and was range
    m_lastRunLoadedAuto = -2; // Ensures negative if +1 to not be valid
    m_wasLastAutoRange = false;
  }
}

/**
 * This timer runs every second when we are watching a directory.
 * If any changes have occurred in the meantime, reload.
 * @param timeup :: [input] Qt timer event (not used)
 */
void ALCDataLoadingPresenter::timerEvent(QTimerEvent *timeup) {
  Q_UNUSED(timeup); // Only have one timer so do not need this

  // Check if there are changes to watched directory
  if (m_directoryChanged.load()) {
    // Need to add most recent file to add to list
    ALCLatestFileFinder finder(m_view->getFirstFile());
    const auto latestFile = finder.getMostRecentFile();

    // Check if file found
    if (latestFile.empty()) {
      // Could not find file this time, but don't reset directory changed flag
      return;
    }
    // If not currently loading load new files
    if (!isLoading()) {
      // add to list set text with search
      const auto oldRuns = m_view->getFiles();
      if (std::find(oldRuns.begin(), oldRuns.end(), latestFile) == oldRuns.end()) {
        // Get old text
        auto newText = m_view->getRunsText();

        // Extract run number from latest file
        auto runNumber = extractRunNumber(latestFile);

        // If new run number is less then error
        if (runNumber <= m_lastRunLoadedAuto) {
          // Is error but continue to watch
          return;
        } else if (m_lastRunLoadedAuto + 1 == runNumber) {
          // Add as range
          // Check if last added was a range
          if (m_wasLastAutoRange.load()) {
            // Remove last run number from text
            newText = newText.substr(0, newText.find_last_of('-'));
          }
          newText += "-" + std::to_string(runNumber);
          m_wasLastAutoRange = true;
        } else {
          // Add as comma
          newText += "," + std::to_string(runNumber);
          m_wasLastAutoRange = false;
        }
        m_filesLoaded.push_back(latestFile);
        m_lastRunLoadedAuto = runNumber;
        // Set text without search, call manual load
        m_view->setRunsTextWithoutSearch(newText);
        try {
          load(m_filesLoaded);
        } catch (const std::runtime_error &loadError) {
          // Stop watching and display error
          m_directoryChanged = false;
          m_view->enableAll();
          m_loadingData = false;
          m_wasLastAutoRange = false;
          m_lastRunLoadedAuto = -2;
          m_view->displayError(loadError.what());
          m_view->toggleRunsAutoAdd(false);
        }
      }

      m_directoryChanged = false;
    }
  }
}

/**
 * Called when a user presses the Period Info button
 * Shows the widget, if the widget is already on show raise to the top
 */
void ALCDataLoadingPresenter::handlePeriodInfoClicked() {
  m_periodInfo->show();
  m_periodInfo->raise();
}

/**
 * Update the Muon Period Info widget with the latest period info from the given workspace
 * @param ws :: The workspace to read the period info from
 */
void ALCDataLoadingPresenter::updateAvailablePeriodInfo(const MatrixWorkspace_sptr &ws) {

  // Clear any current information
  m_periodInfo->clear();

  // Read in all logs and add to widget
  m_periodInfo->addInfo(ws);
  m_periodInfo->setWidgetTitleRuns(m_view->getInstrument() + m_view->getRunsText());
}

} // namespace MantidQt::CustomInterfaces

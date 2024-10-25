// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Strings.h"

#include "ALCLatestFileFinder.h"

#include "Poco/File.h"
#include <Poco/ActiveResult.h>
#include <Poco/Path.h>
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

namespace MantidQt::CustomInterfaces {
ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView *view, std::unique_ptr<ALCDataLoadingModel> model)
    : m_view(view), m_model(std::move(model)), m_directoryChanged(false), m_lastRunLoadedAuto(-2), m_filesLoaded(),
      m_wasLastAutoRange(false), m_previousFirstRun("") {}

void ALCDataLoadingPresenter::initialize() {
  m_view->initialize();
  m_view->subscribePresenter(this);
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
    m_view->getPeriodInfo()->clear();
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
    m_model->setLoadingData(false);
  }
}

/**
 * Load new data and update the view accordingly
 * @param files :: [input] range of files (user-specified or auto generated)
 */
void ALCDataLoadingPresenter::load(const std::vector<std::string> &files) {
  m_model->setLoadingData(true);
  m_view->disableAll();

  // Before loading, check custom grouping (if used) is sensible
  const bool groupingOK = checkCustomGrouping();
  if (!groupingOK) {
    throw std::runtime_error("Custom grouping not valid (bad format or detector numbers)");
  }

  try {
    m_model->load(files, m_view);
    // Plot spectrum 0. It is either red period (if subtract is unchecked) or
    // red - green (if subtract is checked)
    m_view->setDataCurve(m_model->getLoadedData());

  } catch (const std::invalid_argument &e) {
    throw std::runtime_error(e.what()); // Caught in handle load request
  } catch (std::exception &e) {
    throw std::runtime_error(e.what()); // Caught in handle load request
  }
  m_view->enableAll();
  m_model->setLoadingData(false);
}

void ALCDataLoadingPresenter::updateAvailableInfo() {
  Workspace_sptr loadedWs;

  try {
    m_model->setWsForMuonInfo(m_view->getFirstFile());
  } catch (const std::exception &error) {
    m_view->setAvailableInfoToEmpty();
    throw std::runtime_error(error.what());
  }
  m_view->setPath(getPathFromFiles());
  m_view->setAvailableLogs(m_model->getLogs());
  m_view->setAvailablePeriods(m_model->getPeriods());

  // If single period, enable alpha, otherwise disable
  if (m_model->getPeriods().size() == 1) {
    m_view->enableAlpha(true);
    m_view->setAlphaValue("1.0");
    m_view->showAlphaMessage(false);
  } else {
    m_view->enableAlpha(false);
    m_view->showAlphaMessage(true);
  }

  updateAvailablePeriodInfo(m_model->getWsForMuonInfo());

  // Set time limits if this is the first data loaded (will both be zero)
  if (auto timeLimits = m_view->timeRange()) {
    if (std::abs(timeLimits->first) < 0.0001 && std::abs(timeLimits->second) < 0.0001) {
      m_view->setTimeLimits(m_model->getMinTime(), m_model->getWsForMuonInfo()->x(0).back());
    }
  }
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

MatrixWorkspace_sptr ALCDataLoadingPresenter::exportWorkspace() { return m_model->exportWorkspace(); }

void ALCDataLoadingPresenter::setData(const MatrixWorkspace_sptr &data) {
  if (data) {
    m_model->setLoadedData(data);
    m_view->setDataCurve(data);
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
                    [this](const auto det) { return det < 0 || det > static_cast<int>(m_model->getNumDetectors()); })) {
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
bool ALCDataLoadingPresenter::isLoading() const { return m_model->getLoadingData(); }
/**
 * Cancels current loading algorithm
 */
void ALCDataLoadingPresenter::cancelLoading() const { m_model->cancelLoading(); }

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

/**
 * The watched directory has been changed - update flag.
 */
void ALCDataLoadingPresenter::updateDirectoryChangedFlag() { m_directoryChanged = true; }

void ALCDataLoadingPresenter::handleStartWatching(bool watch) {
  if (watch) {
    // Get path to watch and add to watcher
    const auto path = m_view->getPath();
    m_view->getFileSystemWatcher()->addPath(QString::fromStdString(path));
    // start a timer that executes every second
    m_view->getTimer()->start(1000);
  } else {
    // Check if watcher has a directory, then remove all
    if (!m_view->getFileSystemWatcher()->directories().empty()) {
      m_view->getFileSystemWatcher()->removePaths(m_view->getFileSystemWatcher()->directories());
    }
    // Stop timer
    m_view->getTimer()->stop();
    m_lastRunLoadedAuto = -2; // Ensures negative if +1 to not be valid
    m_wasLastAutoRange = false;
  }
}

/**
 * This timer runs every second when we are watching a directory.
 * If any changes have occurred in the meantime, reload.
 */
void ALCDataLoadingPresenter::handleTimerEvent() {

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
        auto runNumber = m_model->extractRunNumber(latestFile);

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
          m_model->setLoadingData(false);
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
  m_view->getPeriodInfo()->show();
  m_view->getPeriodInfo()->raise();
}

/**
 * Update the Muon Period Info widget with the latest period info from the given workspace
 * @param ws :: The workspace to read the period info from
 */
void ALCDataLoadingPresenter::updateAvailablePeriodInfo(const MatrixWorkspace_sptr &ws) {
  // Clear any current information
  m_view->getPeriodInfo()->clear();
  // Read in all logs and add to widget
  m_view->getPeriodInfo()->addInfo(ws);
  m_view->getPeriodInfo()->setWidgetTitleRuns(m_view->getInstrument() + m_view->getRunsText());
}
} // namespace MantidQt::CustomInterfaces

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingPresenter.h"

namespace {
const int RUNS_WARNING_LIMIT = 200;
// must include the "."
const std::vector<std::string> ADDITIONAL_EXTENSIONS{".nxs", ".nxs_v2", ".bin"};
} // namespace

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {
ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView *view, std::unique_ptr<ALCDataLoadingModel> model)
    : m_view(view), m_model(std::move(model)) {}

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
  const bool groupingOK = m_model->checkCustomGrouping(m_view->detectorGroupingType(), m_view->getForwardGrouping(),
                                                       m_view->getBackwardGrouping());
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
  m_view->setPath(m_model->getPathFromFiles(m_view->getFiles()));
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
void ALCDataLoadingPresenter::setDirectoryChanged(bool hasDirectoryChanged) {
  m_model->setDirectoryChanged(hasDirectoryChanged);
}

void ALCDataLoadingPresenter::handleWatcherStopped() { m_model->updateAutoLoadCancelled(); }

/**
 * This timer runs every second when we are watching a directory.
 * If any changes have occurred in the meantime, reload.
 */
void ALCDataLoadingPresenter::handleTimerEvent() {

  bool filesLoadedIntoModel =
      m_model->loadFilesFromWatchingDirectory(m_view->getFirstFile(), m_view->getFiles(), m_view->getRunsText());

  if (filesLoadedIntoModel) {
    // Set text without search, call manual load
    m_view->setRunsTextWithoutSearch(m_model->getRunsText());
    try {
      load(m_model->getFilesLoaded());

    } catch (const std::runtime_error &loadError) {
      // Stop watching and display error
      m_model->setDirectoryChanged(false);
      m_view->enableAll();
      m_model->setLoadingData(false);
      m_model->updateAutoLoadCancelled();
      m_view->displayError(loadError.what());
      m_view->toggleRunsAutoAdd(false);
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

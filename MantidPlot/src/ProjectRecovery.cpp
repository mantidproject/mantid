#include "ProjectRecovery.h"

#include "ApplicationWindow.h"
#include "Folder.h"
#include "Process.h"
#include "ProjectSerialiser.h"
#include "ScriptingWindow.h"
#include "ProjectRecoveryGUIs/ProjectRecoveryPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UsageService.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Environment.h>
#include <Poco/NObserver.h>
#include <Poco/Path.h>
#include <Poco/Process.h>

#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QString>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>

#ifdef _WIN32
#define pid_t int
#endif
namespace {
Mantid::Kernel::Logger g_log("ProjectRecovery");

// Config helper methods
template <typename T>
boost::optional<T> getConfigValue(const std::string &key) {
  return Mantid::Kernel::ConfigService::Instance().getValue<T>(key);
}

boost::optional<bool> getConfigBool(const std::string &key) {
  return Mantid::Kernel::ConfigService::Instance().getValue<bool>(key);
}

/// Returns a string to the folder it should output to
std::string getRecoveryFolderOutput() {
  static std::string appData =
      Mantid::Kernel::ConfigService::Instance().getAppDataDir();
  static std::string hostname = Poco::Environment::nodeName();
  static std::string pid = std::to_string(Process::getProcessID());

  static std::string recoverFolder =
      appData + "/recovery/" + hostname + '/' + pid + '/';
  return recoverFolder;
}

/// Returns a string to the current top level recovery folder
std::string getRecoveryFolderCheck() {
  static std::string appData =
      Mantid::Kernel::ConfigService::Instance().getAppDataDir();
  static std::string hostname = Poco::Environment::nodeName();

  static std::string recoverFolder = appData + "/recovery/" + hostname + '/';
  return recoverFolder;
}

/// Determines if a process ID is being used
bool isPIDused(pid_t pID) {
  if (pID <= 0) {
    return false;
  }
// For Windows:
#if defined(_WIN32) || defined(_WIN64)
  HANDLE handle = OpenProcess(SYNCHRONIZE, false, pID);
  if (!handle) {
    return false;
  } else {
    CloseHandle(handle);
    return true;
  }
#endif
// For Linux:
#if defined(__linux__) || defined(__APPLE__)
  // check if pid exists
  return (0 == kill(pID, 0));
#endif
}

std::vector<Poco::Path>
getListOfFoldersInDirectory(const std::string &recoveryFolderPath) {
  Poco::Path recoveryPath;

  if (!recoveryPath.tryParse(recoveryFolderPath) ||
      !Poco::File(recoveryPath).exists()) {
    // Folder may not exist yet
    g_log.debug("Project Saving: Working folder does not exist");
    return {};
  }

  std::vector<Poco::Path> folderPaths;

  Poco::DirectoryIterator dirIterator(recoveryFolderPath);
  Poco::DirectoryIterator end;
  // Find all the folders which exist in this folder
  while (dirIterator != end) {
    std::string iterPath = recoveryFolderPath + dirIterator.name() + '/';
    Poco::Path foundPath(iterPath);

    if (foundPath.isDirectory()) {
      folderPaths.emplace_back(std::move(foundPath));
    }
    ++dirIterator;
  }

  return folderPaths;
}

std::vector<int> orderProcessIDs(std::vector<Poco::Path> paths) {
  std::vector<int> returnValues;
  // Sort the paths by last modified
  std::sort(paths.begin(), paths.end(),
            [](const Poco::Path &a, const Poco::Path &b) {
              Poco::File a1(a);
              Poco::File b1(b);
              // Last modified is first!
              return a1.getLastModified() > b1.getLastModified();
            });

  for (auto c : paths) {
    returnValues.emplace_back(std::stoi(c.directory(c.depth() - 1)));
  }
  return returnValues;
}

/// Returns a string to the folder that should be recovered
std::string getRecoveryFolderLoad() {
  std::string recoverFolder = getRecoveryFolderCheck();
  // Get the PIDS
  std::vector<Poco::Path> possiblePidsPaths =
      getListOfFoldersInDirectory(recoverFolder);
  if (possiblePidsPaths.size() == 0) {
    throw std::runtime_error(
        "Project Recovery: Load failed attempted to find potential unused pid "
        "but none were found after successful check");
  }
  // Order pids based on date last modified descending
  std::vector<int> possiblePids = orderProcessIDs(possiblePidsPaths);
  // check if pid exists
  for (auto c : possiblePids) {
    if (!isPIDused(c)) {
      // It doesn't exist so return
      return recoverFolder.append(std::to_string(c) + "/");
    }
  }
  // Throw if it gets to this point and hasn't found one.
  throw std::runtime_error(
      "Project Recovery: Load failed attempted to find potential unused pid "
      "but none were found after successful check");
}

/// Gets a formatted timestamp
std::string getTimeStamp() {
  const char *formatSpecifier = "%Y-%m-%dT%H-%M-%S";
  auto time = std::time(nullptr);
  auto localTime = std::localtime(&time);

#if __GNUG__ && __GNUG__ < 5
  // Have to workaround GCC 4 not having std::put_time on RHEL7
  // this ifdef can be removed when RHEL7 uses a newer compiler
  char timestamp[20];
  if (strftime(timestamp, sizeof(timestamp), formatSpecifier, localTime) > 0) {
    return {timestamp};
  }

  return {};
#else
  std::ostringstream timestamp;
  timestamp << std::put_time(localTime, formatSpecifier);
  return timestamp.str();
#endif
}

/// Returns a string to the current timestamped recovery folder
Poco::Path getOutputPath() {

  auto timestamp = getTimeStamp();
  auto timestampedPath = getRecoveryFolderOutput().append(timestamp);

  return Poco::Path{timestampedPath};
}

std::vector<Poco::Path>
getRecoveryFolderCheckpoints(const std::string &recoveryFolderPath) {
  std::vector<Poco::Path> folderPaths =
      getListOfFoldersInDirectory(recoveryFolderPath);

  // Ensure the oldest is first in the vector
  std::sort(folderPaths.begin(), folderPaths.end(),
            [](const Poco::Path &a, const Poco::Path &b) {
              return a.toString() < b.toString();
            });

  return folderPaths;
}

const std::string OUTPUT_PROJ_NAME = "recovery.mantid";

// Config keys
const std::string SAVING_ENABLED_CONFIG_KEY = "projectRecovery.enabled";
const std::string SAVING_TIME_KEY = "projectRecovery.secondsBetween";
const std::string NO_OF_CHECKPOINTS_KEY = "projectRecovery.numberOfCheckpoints";

// Config values
bool SAVING_ENABLED =
    getConfigBool(SAVING_ENABLED_CONFIG_KEY).get_value_or(false);
const int SAVING_TIME =
    getConfigValue<int>(SAVING_TIME_KEY).get_value_or(60); // Seconds
const int NO_OF_CHECKPOINTS =
    getConfigValue<int>(NO_OF_CHECKPOINTS_KEY).get_value_or(5);

// Implementation variables
const std::chrono::seconds TIME_BETWEEN_SAVING(SAVING_TIME);

} // namespace

namespace MantidQt {

/**
 * Constructs a new ProjectRecovery, a class which encapsulates
 * a background thread to save periodically. This does not start the
 * background thread though
 *
 * @param windowHandle :: Pointer to the main application window
 */
ProjectRecovery::ProjectRecovery(ApplicationWindow *windowHandle)
    : m_backgroundSavingThread(), m_stopBackgroundThread(true),
      m_configKeyObserver(*this, &ProjectRecovery::configKeyChanged),
      m_windowPtr(windowHandle),
      m_recoveryGui(this) {}

/// Destructor which also stops any background threads currently in progress
ProjectRecovery::~ProjectRecovery() { stopProjectSaving(); }

void ProjectRecovery::attemptRecovery() {
  bool failed = m_recoveryGui.startRecoveryView();

  if (failed) {
    m_recoveryGui.startRecoveryFailure();
  }
}

bool ProjectRecovery::checkForRecovery() const noexcept {
  try {
    const auto checkpointPaths =
        getRecoveryFolderCheckpoints(getRecoveryFolderCheck());
    return checkpointPaths.size() != 0 &&
           (checkpointPaths.size() > Process::numberOfMantids());
  } catch (...) {
    g_log.warning("Project Recovery: Caught exception whilst attempting to "
                  "check for existing recovery");
    return false;
  }
}

bool ProjectRecovery::clearAllCheckpoints() const noexcept {
  try {
    deleteExistingCheckpoints(0);
    return true;
  } catch (...) {
    g_log.warning("Project Recovery: Caught exception whilst attempting to "
                  "clear existing checkpoints.");
    return false;
  }
}

bool ProjectRecovery::clearAllCheckpoints(Poco::Path path) const noexcept {
  try {
    Poco::File(path).remove(true);
    return true;
  } catch (...) {
    g_log.warning("Project Recovery: Caught exception whilst attempting to "
                  "clear existing checkpoints.");
    return false;
  }
}

bool ProjectRecovery::clearAllUnusedCheckpoints() const noexcept {
  try {
    deleteExistingUnusedCheckpoints(0);
    return true;
  } catch (...) {
    g_log.warning("Project Recovery: Caught exception whilst attempting to "
                  "clear existing checkpoints.");
    return false;
  }
}

/// Returns a background thread with the current object captured inside it
std::thread ProjectRecovery::createBackgroundThread() {
  // Using a lambda helps the compiler deduce the this pointer
  // otherwise the resolution is ambiguous
  return std::thread([this] { projectSavingThreadWrapper(); });
}

/// Callback for POCO when a config change had fired for the enabled key
void ProjectRecovery::configKeyChanged(
    Mantid::Kernel::ConfigValChangeNotification_ptr notif) {
  if (notif->key() != (SAVING_ENABLED_CONFIG_KEY)) {
    return;
  }

  if (notif->curValue() == "True") {
    SAVING_ENABLED = true;
    startProjectSaving();
  } else {
    SAVING_ENABLED = false;
    stopProjectSaving();
  }
}

void ProjectRecovery::compileRecoveryScript(const Poco::Path &inputFolder,
                                            const Poco::Path &outputFile) {
  const std::string algName = "OrderWorkspaceHistory";
  auto alg =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName, 1);
  alg->initialize();
  alg->setChild(true);
  alg->setRethrows(true);
  alg->setProperty("RecoveryCheckpointFolder", inputFolder.toString());
  alg->setProperty("OutputFilepath", outputFile.toString());
  alg->execute();

  g_log.notice("Saved your recovery script to:\n" + outputFile.toString());
}

/**
 * Deletes existing checkpoints, oldest first, in the recovery
 * folder. This is based on the configuration key which
 * indicates how many points to keep
 */
void ProjectRecovery::deleteExistingCheckpoints(
    size_t checkpointsToKeep) const {
  const auto folderPaths =
      getRecoveryFolderCheckpoints(getRecoveryFolderOutput());

  size_t numberOfDirsPresent = folderPaths.size();
  if (numberOfDirsPresent <= checkpointsToKeep) {
    // Nothing to do
    return;
  }

  size_t checkpointsToRemove = numberOfDirsPresent - checkpointsToKeep;
  bool recurse = true;
  for (size_t i = 0; i < checkpointsToRemove; i++) {
    Poco::File(folderPaths[i]).remove(recurse);
  }
}

void ProjectRecovery::deleteExistingUnusedCheckpoints(
    size_t checkpointsToKeep) const {
  std::string recoverFolder = getRecoveryFolderCheck();
  // Get the PIDS
  std::vector<Poco::Path> possiblePidsPaths =
      getListOfFoldersInDirectory(recoverFolder);
  if (possiblePidsPaths.size() == 0) {
    throw std::runtime_error(
        "Project Recovery: Load failed attempted to find potential unused pid "
        "but none were found after successful check");
  }
  // Order pids based on date last modified descending
  std::vector<int> possiblePids = orderProcessIDs(possiblePidsPaths);
  // check if pid exists
  std::vector<std::string> folderPaths;
  for (auto i = 0u; i < possiblePids.size(); ++i) {
    if (!isPIDused(possiblePids[i])) {
      std::string folder = recoverFolder;
      folder.append(std::to_string(possiblePids[i]) + "/");
      folderPaths.emplace_back(folder);
    }
  }

  size_t numberOfDirsPresent = folderPaths.size();
  if (numberOfDirsPresent <= checkpointsToKeep) {
    // Nothing to do
    return;
  }

  size_t checkpointsToRemove = numberOfDirsPresent - checkpointsToKeep;
  bool recurse = true;
  for (size_t i = 0; i < checkpointsToRemove; i++) {
    Poco::File(folderPaths[i]).remove(recurse);
  }
}

/// Starts a background thread which saves out the project periodically
void ProjectRecovery::startProjectSaving() {
  // Close the existing thread first
  stopProjectSaving();

  if (!SAVING_ENABLED) {
    return;
  }

  // Spin up a new thread
  {
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_stopBackgroundThread = false;
  }

  m_backgroundSavingThread = createBackgroundThread();
}

/// Stops any existing background threads which are running
void ProjectRecovery::stopProjectSaving() {
  {
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_stopBackgroundThread = true;
    m_threadNotifier.notify_all();
  }

  if (m_backgroundSavingThread.joinable()) {
    m_backgroundSavingThread.detach();
  }
}

/**
 * Asynchronously loads a recovery checkpoint by opening
 * a scripting window to the ordered workspace
 * history file, then execute it. When this finishes the
 * project loading mechanism is invoked in the main GUI thread
 * to recreate all Qt objects / widgets
 *
 * @param recoveryFolder : The checkpoint folder
 */
void ProjectRecovery::loadRecoveryCheckpoint(const Poco::Path &recoveryFolder) {
  ScriptingWindow *scriptWindow = m_windowPtr->getScriptWindowHandle();
  if (!scriptWindow) {
    throw std::runtime_error("Could not get handle to scripting window");
  }

  // Ensure the window repaints so it doesn't appear frozen before exec
  scriptWindow->executeCurrentTab(Script::ExecutionMode::Serialised);
  if (scriptWindow->getSynchronousErrorFlag()) {
    // We failed to run the whole script
    // Note: We must NOT throw from the method for excepted failures,
    // since doing so will cause the application to terminate from a uncaught
    // exception
    g_log.error("Project recovery script did not finish. Your work has been "
                "partially recovered.");
    this->startProjectSaving();
    return;
  }
  g_log.notice("Re-opening GUIs");

  auto projectFile = Poco::Path(recoveryFolder).append(OUTPUT_PROJ_NAME);

  bool loadCompleted = false;
  if (!QMetaObject::invokeMethod(
          m_windowPtr, "loadProjectRecovery", Qt::BlockingQueuedConnection,
          Q_RETURN_ARG(bool, loadCompleted),
          Q_ARG(const std::string, projectFile.toString()))) {
    this->startProjectSaving();
    throw std::runtime_error("Project Recovery: Failed to load project "
                             "windows - Qt binding failed");
  }

  if (!loadCompleted) {
    g_log.warning("Loading failed to recovery everything completely");
    this->startProjectSaving();
    return;
  }
  g_log.notice("Project Recovery finished");

  // Restart project recovery when the async part finishes
  clearAllCheckpoints(Poco::Path(recoveryFolder).popDirectory());
  startProjectSaving();
} // namespace MantidQt

/**
 * Compiles the project recovery script from a given checkpoint
 * folder and opens this in the script editor
 *
 * @param inputFolder : The folder containing the checkpoint to recover
 * @param historyDest : Where to save the ordered history
 * @throws If a handle to the scripting window cannot be obtained
 */
void ProjectRecovery::openInEditor(const Poco::Path &inputFolder,
                                   const Poco::Path &historyDest) {
  compileRecoveryScript(inputFolder, historyDest);

  // Force application window to create the script window first
  const bool forceVisible = true;
  m_windowPtr->showScriptWindow(forceVisible);

  ScriptingWindow *scriptWindow = m_windowPtr->getScriptWindowHandle();
  if (!scriptWindow) {
    throw std::runtime_error("Could not get handle to scripting window");
  }

  scriptWindow->open(QString::fromStdString(historyDest.toString()));
}

/// Top level thread wrapper which catches all exceptions to gracefully handle
/// them
void ProjectRecovery::projectSavingThreadWrapper() {
  try {
    projectSavingThread();
  } catch (Mantid::API::Algorithm::CancelException &) {
    return;
  } catch (std::exception const &e) {
    std::string preamble("Project recovery has stopped. Please report"
                         " this to the development team.\nException:\n");
    g_log.warning(preamble + e.what());
  } catch (...) {
    g_log.warning("Project recovery has stopped. Please report"
                  " this to the development team.");
  }
}

/**
 * Main thread body which is run to save out projects. A member mutex is
 * locked and monitored on a timeout to indicate if the thread should
 * exit early. After the timeout elapses, if the thread has not been
 * requested to exit, it will save the project out
 */
void ProjectRecovery::projectSavingThread() {
  while (!m_stopBackgroundThread) {
    { // Ensure the lock only exists as long as the conditional variable
      std::unique_lock<std::mutex> lock(m_notifierMutex);
      // The condition variable releases the lock until the var changes
      if (m_threadNotifier.wait_for(lock, TIME_BETWEEN_SAVING, [this]() {
            return m_stopBackgroundThread.load();
          })) {
        // Exit thread
        g_log.debug("Project Recovery: Stopping background saving thread");
        return;
      }
    }
    this->saveAll();
  }
}
/**
 * Saves open all open windows using the main GUI thread
 *
 * @param projectDestFile :: The full path to write to
 * @throws If saving fails in the main GUI thread
 */
void ProjectRecovery::saveOpenWindows(const std::string &projectDestFile,
                                      bool autoSave) {
  bool saveCompleted = false;
  if (autoSave) {
    if (!QMetaObject::invokeMethod(m_windowPtr, "saveProjectRecovery",
                                   Qt::BlockingQueuedConnection,
                                   Q_RETURN_ARG(bool, saveCompleted),
                                   Q_ARG(const std::string, projectDestFile))) {
      throw std::runtime_error("Project Recovery: Failed to save project "
                               "windows - Qt binding failed");
    }
  } else {
    // Only use this if it is called from the python interface/error reporter
    saveCompleted = m_windowPtr->saveProjectRecovery(projectDestFile);
  }

  if (!saveCompleted) {
    throw std::runtime_error(
        "Project Recovery: Failed to write out project file");
  }
}

/**
 * Saves all workspace histories by using an external python script
 *
 * @param historyDestFolder:: The folder to write all histories to
 * @throw If saving fails in the script
 */
void ProjectRecovery::saveWsHistories(const Poco::Path &historyDestFolder) {
  const auto &ads = Mantid::API::AnalysisDataService::Instance();

  // Hold a copy to the shared pointers so they do not get deleted under us
  std::vector<boost::shared_ptr<Mantid::API::Workspace>> wsHandles =
      ads.getObjects(Mantid::Kernel::DataServiceHidden::Include);

  if (wsHandles.empty()) {
    return;
  }

  static auto startTime =
      Mantid::Kernel::UsageService::Instance().getStartTime().toISO8601String();

  const std::string algName = "GeneratePythonScript";
  auto alg =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName, 1);
  alg->setChild(true);
  alg->setLogging(false);

  for (auto i = 0u; i < wsHandles.size(); ++i) {
    std::string filename = std::to_string(i) + ".py";

    Poco::Path destFilename = historyDestFolder;
    destFilename.append(filename);

    alg->initialize();
    alg->setLogging(false);
    alg->setProperty("AppendTimestamp", true);
    alg->setProperty("InputWorkspace", wsHandles[i]);
    alg->setPropertyValue("Filename", destFilename.toString());
    alg->setPropertyValue("StartTimestamp", startTime);
    alg->setProperty("IgnoreTheseAlgs", m_algsToIgnore);

    alg->execute();
  }
}

void ProjectRecovery::removeOlderCheckpoints() {
  // Currently set to a month in microseconds
  const int64_t timeToDeleteAfter = 2592000000000;
  std::string recoverFolder = getRecoveryFolderCheck();
  // Get the PIDS
  std::vector<Poco::Path> possiblePidsPaths =
      getListOfFoldersInDirectory(recoverFolder);
  // Order pids based on date last modified descending
  std::vector<int> possiblePids = orderProcessIDs(possiblePidsPaths);
  // check if pid exists
  std::vector<std::string> folderPaths;
  for (auto i = 0u; i < possiblePids.size(); ++i) {
    if (!isPIDused(possiblePids[i])) {
      std::string folder = recoverFolder;
      folder.append(std::to_string(possiblePids[i]) + "/");
      if (olderThanAGivenTime(Poco::Path(folder), timeToDeleteAfter)) {
        folderPaths.emplace_back(folder);
      }
    }
  }

  bool recurse = true;
  for (size_t i = 0; i < folderPaths.size(); i++) {
    Poco::File(folderPaths[i]).remove(recurse);
  }
}

bool ProjectRecovery::olderThanAGivenTime(const Poco::Path &path,
                                          int64_t elapsedTime) {
  return Poco::File(path).getLastModified().isElapsed(elapsedTime);
}

/**
 * @brief A function that brings the two seperate save methods together
 * This won't run if it is locked by the background thread but then it saving
 * Anyway so thats no issue.
 */
void ProjectRecovery::saveAll(bool autoSave) {
  // "Timeout" - Save out again
  const auto &ads = Mantid::API::AnalysisDataService::Instance();
  if (ads.size() == 0) {
    g_log.debug("Nothing to save");
    return;
  }

  g_log.debug("Project Recovery: Saving started");

  const auto basePath = getOutputPath();
  Poco::File(basePath).createDirectories();

  saveWsHistories(basePath);
  auto projectFile = Poco::Path(basePath).append(OUTPUT_PROJ_NAME);
  saveOpenWindows(projectFile.toString(), autoSave);

  // Purge any excessive folders
  deleteExistingCheckpoints(NO_OF_CHECKPOINTS);
  g_log.debug("Project Recovery: Saving finished");
}

std::string ProjectRecovery::getRecoveryFolderOutputPR() {
  return getRecoveryFolderOutput();
}
} // namespace MantidQt

#include "ProjectRecoveryThread.h"

#include "ApplicationWindow.h"
#include "Folder.h"
#include "ProjectSerialiser.h"
#include "globals.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"

#include "boost/optional.hpp"

#include "Poco/DirectoryIterator.h"
#include "Poco/NObserver.h"
#include "Poco/Path.h"
#include "qmetaobject.h"

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace {
// Config helper methods

template <typename T>
boost::optional<T> getConfigValue(const std::string &key) {
	T returnedValue;
	
	int valueIsGood =
      Mantid::Kernel::ConfigService::Instance().getValue<T>(
          SAVING_ENABLED_CONFIG_KEY, returnedValue);

  if (valueIsGood != 1) {
    return boost::optional<T>{};
  }

  return boost::optional<T>{returnedValue};
}

boost::optional<bool> getConfigBool(const std::string &key) {
  auto returnedValue = getConfigValue<std::string>(key);
  if (!returnedValue.is_initialized()) {
    return boost::optional<bool>{};
  }

  return returnedValue->find("true") != std::string::npos;
}

std::string getRecoveryFolder() {
  static std::string recoverFolder =
      Mantid::Kernel::ConfigService::Instance().getAppDataDir() + "/recovery/";
  return recoverFolder;
}

std::string getOutputPath() {
  auto time = std::time(nullptr);
  auto localTime = std::localtime(&time);

  std::ostringstream timestamp;
  timestamp << std::put_time(localTime, "%Y-%m-%d %H-%M-%S");
  auto timestampedPath = getRecoveryFolder().append(timestamp.str());

  return timestampedPath;
}

const std::string OUTPUT_PROJ_NAME = "recovery.mantid" ;

// Config keys
const std::string SAVING_ENABLED_CONFIG_KEY = "projectRecovery.enabled";
const std::string SAVING_TIME_KEY = "projectRecovery.secondsBetween";
const std::string NO_OF_CHECKPOINTS_KEY = "projectRecovery.numberOfCheckpoints";

// Config values
const bool SAVING_ENABLED =
getConfigBool(SAVING_ENABLED_CONFIG_KEY).get_value_or(false);
const int SAVING_TIME =
getConfigValue<int>(SAVING_TIME_KEY).get_value_or(60); // Seconds
const int NO_OF_CHECKPOINTS =
getConfigValue<int>(NO_OF_CHECKPOINTS_KEY).get_value_or(5);

Mantid::Kernel::Logger g_log("Project Recovery Thread");
const std::chrono::seconds TIME_BETWEEN_SAVING =
std::chrono::seconds(SAVING_TIME);

} // namespace

namespace MantidQt {
namespace API {

ProjectRecoveryThread::ProjectRecoveryThread(ApplicationWindow *windowHandle)
    : m_backgroundSavingThread(), m_stopBackgroundThread(true),
      m_configKeyObserver(*this, &ProjectRecoveryThread::configKeyChanged),
      m_windowPtr(windowHandle) {}

ProjectRecoveryThread::~ProjectRecoveryThread() { stopProjectSaving(); }

std::thread ProjectRecoveryThread::createBackgroundThread() {
  // Using a lambda helps the compiler deduce the this pointer
  // otherwise the resolution is ambiguous
  return std::thread([this] { projectSavingThreadWrapper(); });
}

void ProjectRecoveryThread::configKeyChanged(
    Mantid::Kernel::ConfigValChangeNotification_ptr notif) {
  if (notif->key() != (SAVING_ENABLED_CONFIG_KEY)) {
    return;
  }

  if (notif->curValue() == "True") {
    startProjectSaving();
  } else {
    stopProjectSaving();
  }
}

void ProjectRecoveryThread::deleteExistingCheckpoints(
    size_t checkpointsToKeep) {
  static auto workingFolder = getRecoveryFolder();
  Poco::Path recoveryPath;
  if (!recoveryPath.tryParse(workingFolder)) {
    // Folder may not exist
    g_log.debug("Project Saving: Failed to get working folder whilst deleting "
                "checkpoints");
    return;
  }

  std::vector<Poco::Path> folderPaths;

  Poco::DirectoryIterator dirIterator(recoveryPath);
  Poco::DirectoryIterator end;
  while (dirIterator != end) {
    std::string iterPath = workingFolder + dirIterator.name() + '/';
    Poco::Path foundPath(iterPath);

    if (foundPath.isDirectory()) {
      folderPaths.push_back(std::move(foundPath));
    }
    ++dirIterator;
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

void ProjectRecoveryThread::startProjectSaving() {
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

void ProjectRecoveryThread::stopProjectSaving() {
  {
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_stopBackgroundThread = true;
    m_threadNotifier.notify_all();
  }

  if (m_backgroundSavingThread.joinable()) {
    m_backgroundSavingThread.join();
  }
}

void ProjectRecoveryThread::projectSavingThreadWrapper() {
  try {
    projectSavingThread();
  } catch (std::exception const &e) {
    std::string preamble("Project recovery has stopped. Please report"
                         " this to the development team.\nException:\n");
    g_log.warning(preamble + e.what());
  } catch (...) {
    g_log.warning("Project recovery has stopped. Please report"
                  " this to the development team.");
  }
}

void ProjectRecoveryThread::projectSavingThread() {
  while (!m_stopBackgroundThread) {
    std::unique_lock<std::mutex> lock(m_notifierMutex);
    // The condition variable releases the lock until the var changes
    if (m_threadNotifier.wait_for(lock, TIME_BETWEEN_SAVING, [this]() {
          return m_stopBackgroundThread;
        })) {
      // Exit thread
      g_log.information("Project Recovery: Stopping background saving thread");
      return;
    }

    g_log.information("Project Recovery: Saving started");
    // "Timeout" - Save out again
    // Generate output paths
    const auto basePath = getOutputPath();
    auto projectFile = Poco::Path(basePath).append(OUTPUT_PROJ_NAME);

    // Python's OS module cannot handle Poco's parsed paths
    // so use std::string instead and let OS parse the '/' char on Win
    saveWsHistories(basePath);
    saveOpenWindows(projectFile.toString());

    // Purge any excessive folders
    deleteExistingCheckpoints(NO_OF_CHECKPOINTS);
    g_log.information("Project Recovery: Saving finished");
  }
}

void ProjectRecoveryThread::saveOpenWindows(
    const std::string &projectDestFile) {
  if (!QMetaObject::invokeMethod(m_windowPtr, "saveProjectRecovery",
                                 Qt::QueuedConnection,
                                 Q_ARG(const std::string, projectDestFile))) {
    g_log.warning(
        "Project Recovery: Failed to save project windows - Qt binding failed");
  }
}

void ProjectRecoveryThread::saveWsHistories(
    const std::string &historyDestFolder) {
  QString projectSavingCode =
      "from mantid.simpleapi import write_all_workspaces_histories\n"
      "write_all_workspaces_histories(\"" +
      QString::fromStdString(historyDestFolder) + "\")\n";

  if (!m_windowPtr->runPythonScript(projectSavingCode)) {
    throw std::runtime_error("Project Recovery: Python saving failed");
  }
}

void ProjectRecoveryThread::loadOpenWindows(const std::string &projectFolder) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(m_windowPtr, isRecovery);

  // Use this version of Mantid as the current version field - as recovery
  // across major versions is not an intended use case
  const int fileVersion = 100 * maj_version + 10 * min_version + patch_version;

  projectWriter.load(projectFolder, fileVersion);
}

} // namespace API
} // namespace MantidQt

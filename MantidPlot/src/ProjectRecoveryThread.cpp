#include "ProjectRecoveryThread.h"

#include "ApplicationWindow.h"
#include "Folder.h"
#include "ProjectSerialiser.h"
#include "globals.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"

#include "Poco/NObserver.h"
#include "Poco/Path.h"
#include "qmetaobject.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace {
Mantid::Kernel::Logger g_log("Project Recovery Thread");
const std::chrono::seconds TIME_BETWEEN_SAVING = std::chrono::seconds(30);
const std::string SAVING_ENABLED_CONFIG_KEY = "projectRecovery.enabled";

bool isRecoveryEnabled() {
  std::string isEnabled;
  int valueIsGood = Mantid::Kernel::ConfigService::Instance().getValue<std::string>(
      SAVING_ENABLED_CONFIG_KEY, isEnabled);
  
  return (valueIsGood == 1) && isEnabled.find("true") != std::string::npos;
}

std::string getOutputPath() {
  static bool isInitalised = false;
  static std::string recoverFolder;

  if (!isInitalised) {
    recoverFolder = Mantid::Kernel::ConfigService::Instance().getAppDataDir();
    recoverFolder.append("/recovery");
    isInitalised = true;
  }

  return recoverFolder;
}

std::string getOutputProjectName() { return "recovery.project"; }

} // namespace

namespace MantidQt {
namespace API {

ProjectRecoveryThread::ProjectRecoveryThread(ApplicationWindow *windowHandle)
    : m_backgroundSavingThread(), m_stopBackgroundThread(true),
      m_configKeyObserver(*this, &ProjectRecoveryThread::configKeyChanged),
      m_windowPtr(windowHandle) {

  if (isRecoveryEnabled()) {
    startProjectSaving();
  }
}

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

void ProjectRecoveryThread::startProjectSaving() {
  // Close the existing thread first
  stopProjectSaving();

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
    auto projectFile = Poco::Path(basePath).append(getOutputProjectName());

    // Python's OS module cannot handle Poco's parsed paths
    // so use std::string instead and let OS parse the '/' char on Win
	saveWsHistories(basePath);
    saveOpenWindows(projectFile.toString());
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

  if (!m_windowPtr->runPythonScript(projectSavingCode)){
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

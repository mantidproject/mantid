// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECT_RECOVERY_H_
#define PROJECT_RECOVERY_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/ConfigService.h"
#include "ProjectRecoveryGUIs/ProjectRecoveryPresenter.h"

#include <Poco/NObserver.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

// Forward declarations
class ApplicationWindow;
class Folder;
namespace Poco {
class Path;
}

/** Adapter class which handles saving or restoring project windows

@author David Fairbrother, ISIS, RAL
@date 07/06/2018
*/

namespace MantidQt {
class ProjectRecovery {
public:
  /// Constructor
  explicit ProjectRecovery(ApplicationWindow *windowHandle);

  /// Destructor the ensures background thread stops
  ~ProjectRecovery();

  /// Attempts recovery of the most recent checkpoint
  void attemptRecovery();
  /// Checks if recovery is required
  bool checkForRecovery() const noexcept;

  /// Clears all checkpoints in the existing folder at the given path
  bool clearAllCheckpoints(Poco::Path path) const noexcept;

  /// Clears all checkpoints in the existing folder at the given path
  bool clearAllUnusedCheckpoints() const noexcept;

  /// Starts the background thread
  void startProjectSaving();

  /// Stops the background thread
  void stopProjectSaving();

  /// Saves a project recovery checkpoint
  void saveAll(bool autoSave = true);

  /// get Recovery Folder location
  std::string getRecoveryFolderOutputPR();

  /// Get a list of poco paths based on recoveryFolderPaths' directory
  std::vector<Poco::Path>
  getListOfFoldersInDirectoryPR(const std::string &recoveryFolderPath);

  /// get Recovery Folder to loads location
  std::string getRecoveryFolderLoadPR();

  /// Exposing the getRecoveryFolderCheckpoints function
  std::vector<Poco::Path>
  getRecoveryFolderCheckpointsPR(const std::string &recoveryFolderPath);

  /// Expose the getRecoveryFolderCheck function
  std::string getRecoveryFolderCheckPR();

  /// Loads a recovery checkpoint in the given folder
  bool loadRecoveryCheckpoint(const Poco::Path &path);

  /// Open a recovery checkpoint in the scripting window
  void openInEditor(const Poco::Path &inputFolder,
                    const Poco::Path &historyDest);

  /// Looks at the recovery checkpoints and repairs some faults
  void repairCheckpointDirectory();

private:
  friend class RecoveryThread;
  /// Captures the current object in the background thread
  std::thread createBackgroundThread();

  /// Creates a recovery script based on all .py scripts in a folder
  void compileRecoveryScript(const Poco::Path &inputFolder,
                             const Poco::Path &outputFile);

  /// Deletes oldest checkpoints beyond the maximum number to keep
  void deleteExistingCheckpoints(size_t checkpointsToKeep) const;

  /// Deletes oldest checkpoints beyond the maximum number to keep at the path
  void deleteExistingCheckpoints(size_t checkpointsToKeep,
                                 Poco::Path path) const;

  /// Deletes oldest "unused" checkpoints beyond the maximum number to keep
  void deleteExistingUnusedCheckpoints(size_t checkpointsToKeep) const;

  /// Wraps the thread in a try catch to log any failures
  void projectSavingThreadWrapper();

  /// Main body of saving thread
  void projectSavingThread();

  /// Saves a project recovery file in Mantid
  void saveOpenWindows(const std::string &projectDestFolder,
                       bool autoSave = true);

  /// Saves the current workspace's histories from Mantid
  void saveWsHistories(const Poco::Path &projectDestFile);

  /// Return true if the folder at the end of the path is older than a month.
  bool olderThanAGivenTime(const Poco::Path &path, int64_t elapsedTime);

  /// Finds any checkpoints older than a time defined inside the function and
  /// returns paths to them
  std::vector<std::string>
  findOlderCheckpoints(const std::string &recoverFolder,
                       const std::vector<int> &possiblePids);

  /// Finds any checkpoints containing a Locked file and returns paths to them
  std::vector<std::string>
  findLockedCheckpoints(const std::string &recoverFolder,
                        const std::vector<int> &possiblePids);

  /// Finds any checkpoints believed to be from previous versions of project
  /// recovery and returns them
  std::vector<std::string>
  findLegacyCheckpoints(const std::vector<Poco::Path> &checkpoints);

  /// Takes a list of potentially used PIDs and removes any used PIDs from that
  /// list
  void checkPIDsAreNotInUse(std::vector<int> &possiblePids);

  /// Background thread which runs the saving body
  std::thread m_backgroundSavingThread;

  /// Mutex for conditional variable and background thread flag
  std::mutex m_notifierMutex;

  /// Flag to indicate to the thread to exit
  std::atomic<bool> m_stopBackgroundThread;

  /// Atomic to detect when the thread should fire or exit
  std::condition_variable m_threadNotifier;

  /// Pointer to main GUI window
  ApplicationWindow *m_windowPtr;

  // To ignore a property you need to first put the algorithm name in the first
  // part of the vector for which you want to ignore the property for then the
  // property name in the second part of the vector 0 and 1 as indexes
  // respectively
  std::vector<std::vector<std::string>> m_propertiesToIgnore = {
      {"StartLiveData", "MonitorLiveData"}};

  ProjectRecoveryPresenter *m_recoveryGui;

  std::vector<std::string> m_algsToIgnore = {
      "MonitorLiveData",
      "EnggSaveGSASIIFitResultsToHDF5",
      "EnggSaveSinglePeakFitResultsToHDF5",
      "ExampleSaveAscii",
      "SANSSave",
      "SaveANSTOAscii",
      "SaveAscii",
      "SaveBankScatteringAngles",
      "SaveCSV",
      "SaveCalFile",
      "SaveCanSAS1D",
      "SaveDaveGrp",
      "SaveDetectorsGrouping",
      "SaveDiffCal",
      "SaveDiffFittingAscii",
      "SaveDspacemap",
      "SaveFITS",
      "SaveFocusedXYE",
      "SaveFullprofResolution",
      "SaveGDA",
      "SaveGEMMAUDParamFile",
      "SaveGSASInstrumentFile",
      "SaveGSS",
      "SaveHKL",
      "SaveILLCosmosAscii",
      "SaveISISNexus",
      "SaveIsawDetCal",
      "SaveIsawPeaks",
      "SaveIsawQvector",
      "SaveIsawUB",
      "SaveLauenorm",
      "SaveMD",
      "SaveMDWorkspaceToVTK",
      "SaveMask",
      "SaveNISTDAT",
      "SaveNXSPE",
      "SaveNXTomo",
      "SaveNXcanSAS",
      "SaveNexus",
      "SaveNexusPD",
      "SaveNexusProcessed",
      "SaveOpenGenieAscii",
      "SavePAR",
      "SavePDFGui",
      "SavePHX",
      "SaveParameterFile",
      "SavePlot1D",
      "SavePlot1DAsJson",
      "SaveRKH",
      "SaveReflCustomAscii",
      "SaveReflThreeColumnAscii",
      "SaveReflections",
      "SaveReflectometryAscii",
      "SaveSESANS",
      "SaveSPE",
      "SaveTBL",
      "SaveToSNSHistogramNexus",
      "SaveVTK",
      "SaveVulcanGSS",
      "SaveYDA",
      "SaveZODS"};
};
} // namespace MantidQt

#endif // PROJECT_RECOVERY_H_

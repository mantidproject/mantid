#ifndef PROJECT_RECOVERY_H_
#define PROJECT_RECOVERY_H_

#include "MantidKernel/ConfigService.h"
#include "RecoveryGUIs/ProjectRecoveryPresenter.h"

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

Copyright &copy; 2007-2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
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

  /// Clears all checkpoints in the existing folder
  bool clearAllCheckpoints() const noexcept;

  /// Clears all checkpoints in the existing folder at the given path
  bool clearAllCheckpoints(Poco::Path path) const noexcept;

  /// Clears all checkpoints in the existing folder at the given path
  bool clearAllUnusedCheckpoints() const noexcept;

  /// Starts the background thread
  void startProjectSaving();
  /// Stops the background thread
  void stopProjectSaving();

  /// Removes checkpoints should they be older than a month old.
  void removeOlderCheckpoints();

  /// Saves a project recovery checkpoint
  void saveAll(bool autoSave = true);

private:
  /// Captures the current object in the background thread
  std::thread createBackgroundThread();

  /// Triggers when the config key is updated to a new value
  void configKeyChanged(Mantid::Kernel::ConfigValChangeNotification_ptr notif);

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

  /// Loads a recovery checkpoint in the given folder
  void loadRecoveryCheckpoint(const Poco::Path &path);

  /// Open a recovery checkpoint in the scripting window
  void openInEditor(const Poco::Path &inputFolder,
                    const Poco::Path &historyDest);

  /// Wraps the thread in a try catch to log any failures
  void projectSavingThreadWrapper();

  /// Main body of saving thread
  void projectSavingThread();

  /// Saves a project recovery file in Mantid
  void saveOpenWindows(const std::string &projectDestFolder,
                       bool autoSave = true);

  /// Saves the current workspace's histories from Mantid
  void saveWsHistories(const Poco::Path &projectDestFile);

  // Return true if the folder at the end of the path is older than a month.
  bool olderThanAGivenTime(const Poco::Path &path, int64_t elapsedTime);

  /// Background thread which runs the saving body
  std::thread m_backgroundSavingThread;

  /// Mutex for conditional variable and background thread flag
  std::mutex m_notifierMutex;
  /// Flag to indicate to the thread to exit
  std::atomic<bool> m_stopBackgroundThread;
  /// Atomic to detect when the thread should fire or exit
  std::condition_variable m_threadNotifier;

  /// Config observer to monitor the key
  Poco::NObserver<ProjectRecovery, Mantid::Kernel::ConfigValChangeNotification>
      m_configKeyObserver;

  /// Pointer to main GUI window
  ApplicationWindow *m_windowPtr;

  // The presenter of the recovery guis
  ProjectRecoveryPresenter m_recoveryGui;

  std::vector<std::string> m_algsToIgnore = {
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

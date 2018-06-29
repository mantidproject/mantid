#ifndef PROJECT_RECOVERY_H_
#define PROJECT_RECOVERY_H_

#include "MantidKernel/ConfigService.h"

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
  bool attemptRecovery();
  /// Checks if recovery is required
  bool checkForRecovery() const;

  /// Clears all checkpoints in the existing folder
  void clearAllCheckpoints() const { deleteExistingCheckpoints(0); };

  /// Starts the background thread
  void startProjectSaving();
  /// Stops the background thread
  void stopProjectSaving();

private:
  /// Captures the current object in the background thread
  std::thread createBackgroundThread();

  /// Triggers when the config key is updated to a new value
  void configKeyChanged(Mantid::Kernel::ConfigValChangeNotification_ptr notif);

  /// Creates a recovery script based on all .py scripts in a folder
  void compileRecoveryScript(const Poco::Path &inputFolder, const Poco::Path &outputFile);

  /// Deletes oldest checkpoints beyond the maximum number to keep
  void deleteExistingCheckpoints(size_t checkpointsToKeep) const;

  /// Loads a recovery checkpoint in the given folder - TODO in future PR
  // bool loadRecoveryCheckpoint(const Poco::Path &path); 

  /// Open a recovery checkpoint in the scripting window
  bool openInEditor(const Poco::Path &inputFolder);

  /// Wraps the thread in a try catch to log any failures
  void projectSavingThreadWrapper();

  /// Main body of saving thread
  void projectSavingThread();

  /// Saves a project recovery file in Mantid
  void saveOpenWindows(const std::string &projectDestFolder);

  /// Saves the current workspace's histories from Mantid
  void saveWsHistories(const Poco::Path &projectDestFile);

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
};

} // namespace MantidQt

#endif // PROJECT_RECOVERY_H_

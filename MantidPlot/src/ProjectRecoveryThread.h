#ifndef PROJECT_RECOVERY_THREAD_H_
#define PROJECT_RECOVERY_THREAD_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

// Forward declarations
class ApplicationWindow;
class Folder;

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
namespace API {
class ProjectRecoveryThread {
public:
  /// Constructor
  explicit ProjectRecoveryThread(ApplicationWindow *windowHandle);
  /// Destructor the ensures background thread stops
  ~ProjectRecoveryThread();

  /// Starts the background thread
  void startProjectSaving();
  /// Stops the background thread
  void stopProjectSaving();

private:
  /// Captures the current object in the background thread
  std::thread createBackgroundThread();

  /// Loads a project recovery file back into Mantid
  void loadOpenWindows(const std::string &projectFolder);
  /// Saves a project recovery file in Mantid
  void saveOpenWindows(const std::string &projectDestFolder);
  /// Saves the current workspace's histories from Mantid
  void saveWsHistories(const std::string &projectDestFile);
  /// Main body of saving thread
  void projectSavingThread();

  /// Background thread which runs the saving body
  std::thread m_backgroundSavingThread;

  /// Mutex for conditional variable and background thread flag
  std::mutex m_notifierMutex;
  /// Flag to indicate to the thread to exit
  bool m_stopBackgroundThread;
  /// Atomic to detect when the thread should fire or exit
  std::condition_variable m_threadNotifier;

  /// Pointer to main GUI window
  ApplicationWindow *m_windowPtr;
};

} // namespace API
} // namespace MantidQt

#endif // PROJECT_RECOVERY_THREAD_H_

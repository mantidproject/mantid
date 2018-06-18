#ifndef PROJECT_RECOVERY_THREAD_H_
#define PROJECT_RECOVERY_THREAD_H_

#include <chrono>
#include <thread>
#include <string>

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
  explicit ProjectRecoveryThread(ApplicationWindow *windowHandle);
  ~ProjectRecoveryThread();

  void startProjectSaving();
  void stopProjectSaving();


private:
	std::thread createBackgroundThread();
	void projectSavingThread(bool &runThread);
	void saveOpenWindows(std::string projectFilepath);
	void saveWsHistories(std::string historyFilePath);
	void loadOpenWindows(std::string projectFilePath);

  /// Flag to toggle the threads operation.
  bool m_runProjectSaving; // Does not need to be atomic, as we only read in worker

  std::thread m_backgroundSavingThread;

  ApplicationWindow *m_windowPtr;
};

} // namespace API
} // namespace MantidQt

#endif // PROJECT_RECOVERY_THREAD_H_
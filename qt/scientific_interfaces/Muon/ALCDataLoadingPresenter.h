#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCDataLoadingView.h"

#include <QFileSystemWatcher>
#include <QObject>

#include <atomic>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingPresenter : Presenter for ALC Data Loading step

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingPresenter : public QObject {
  Q_OBJECT

public:
  ALCDataLoadingPresenter(IALCDataLoadingView *view);

  void initialize();

  /// @return Last loaded data workspace
  Mantid::API::MatrixWorkspace_const_sptr loadedData() const {
    return m_loadedData;
  }

  /// @return Loaded data as MatrixWorkspace_sptr
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Sets some data
  void setData(Mantid::API::MatrixWorkspace_const_sptr data);

private slots:
  /// Check file range and call method to load new data
  void handleLoadRequested();

  /// Updates the list of logs and number of periods
  void updateAvailableInfo();

  /// When directory contents change, set flag
  void updateDirectoryChangedFlag(const QString &path);

  /// When "Auto" selected/deselected, start/stop watching directory
  void changeWatchState(int state);

signals:
  /// Signal emitted when data get changed
  void dataChanged();

protected:
  /// Signal emitted when timer event occurs
  void timerEvent(QTimerEvent *timeup) override;

private:
  /// Load new data and update the view accordingly
  void load(const std::string &lastFile);

  /// Start/stop watching directory
  void changeWatchState(bool watching);

  /// Check custom grouping is sensible
  bool checkCustomGrouping();

  /// View which the object works with
  IALCDataLoadingView *const m_view;

  /// Last loaded data workspace
  Mantid::API::MatrixWorkspace_const_sptr m_loadedData;

  /// Watch a directory for changes
  QFileSystemWatcher m_watcher;

  /// Flag to indicate directory has had changes since last load
  std::atomic_bool m_directoryChanged;

  /// ID of timer, if one is running
  int m_timerID;

  /// Number of detectors for current first run
  size_t m_numDetectors;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_ */

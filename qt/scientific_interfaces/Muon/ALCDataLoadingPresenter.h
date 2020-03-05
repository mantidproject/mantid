// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_

#include "DllConfig.h"
#include "IALCDataLoadingView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include <QFileSystemWatcher>
#include <QObject>
#include <atomic>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingPresenter : Presenter for ALC Data Loading step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingPresenter : public QObject {
  Q_OBJECT

public:
  ALCDataLoadingPresenter(IALCDataLoadingView *view);

  void initialize();

  /// @return Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr loadedData() const { return m_loadedData; }

  /// @return Loaded data as MatrixWorkspace_sptr
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Sets some data
  void setData(Mantid::API::MatrixWorkspace_sptr data);

  // Returns a boolean stating whether data is currently being loading
  bool isLoading() const;

  // Cancels current loading algorithm
  void cancelLoading() const;

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

  /// Check the group is valid
  std::string isCustomGroupingValid(const std::string &group, bool &isValid);

  /// View which the object works with
  IALCDataLoadingView *const m_view;

  /// Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr m_loadedData;

  /// Watch a directory for changes
  QFileSystemWatcher m_watcher;

  /// Flag to indicate directory has had changes since last load
  std::atomic_bool m_directoryChanged;

  /// ID of timer, if one is running
  int m_timerID;

  /// Number of detectors for current first run
  size_t m_numDetectors;

  // bool to state whether loading data
  std::atomic_bool m_loadingData;

  // Loading algorithm
  Mantid::API::IAlgorithm_sptr m_LoadingAlg;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_ */

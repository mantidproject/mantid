// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IALCDataLoadingPresenter.h"
#include "IALCDataLoadingView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <atomic>

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingPresenter : Presenter for ALC Data Loading step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingPresenter : IALCDataLoadingPresenter {

public:
  ALCDataLoadingPresenter(IALCDataLoadingView *view);

  void initialize() override;

  /// @return Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr loadedData() const override { return m_loadedData; }

  /// @return Loaded data as MatrixWorkspace_sptr
  Mantid::API::MatrixWorkspace_sptr exportWorkspace() override;

  /// Sets some data
  void setData(const Mantid::API::MatrixWorkspace_sptr &data) override;

  // Returns a boolean stating whether data is currently being loading
  bool isLoading() const override;

  // Cancels current loading algorithm
  void cancelLoading() const override;

  /// Check file range and call method to load new data
  void handleLoadRequested() override;

  /// Updates the list of logs and number of periods
  void updateAvailableInfo() override;

  /// Handle for when runs editing starts
  void handleRunsEditing() override;

  /// Handle for when runs editing finishes
  void handleRunsEditingFinished() override;

  /// Handle for when instrument changed
  void handleInstrumentChanged(const std::string &instrument) override;

  /// Handle for when manage user directories clicked
  void handleManageDirectories() override;

  /// Handle for when runs have been searched for
  void handleRunsFound() override;

  /// When directory contents change, set flag
  void updateDirectoryChangedFlag() override;

  /// Begin/Stop watching path
  void handleStartWatching(bool watch) override;

  /// Handle a user requests to see the period info widget
  void handlePeriodInfoClicked() override;

protected:
  /// Runs every time a timer event occurs
  void timerEvent();

private:
  /// Load new data and update the view accordingly
  void load(const std::vector<std::string> &files);

  /// Check custom grouping is sensible
  bool checkCustomGrouping();

  /// Extract run number as int from file name string
  int extractRunNumber(const std::string &file);

  /// Check the group is valid
  std::string isCustomGroupingValid(const std::string &group, bool &isValid);

  /// Get path from files
  std::string getPathFromFiles() const;

  /// Update info on MuonPeriodInfo widget using sample logs from ws
  void updateAvailablePeriodInfo(const Mantid::API::MatrixWorkspace_sptr &ws);

  /// View which the object works with
  IALCDataLoadingView *const m_view;

  /// Period Info Widget displayed from the view
  std::unique_ptr<MantidQt::MantidWidgets::MuonPeriodInfo> m_periodInfo;

  /// Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr m_loadedData;

  /// Number of detectors for current first run
  size_t m_numDetectors;

  // bool to state whether loading data
  std::atomic_bool m_loadingData;

  // Loading algorithm
  Mantid::API::IAlgorithm_sptr m_LoadingAlg;

  /// Watches the path for changes
  QFileSystemWatcher m_watcher;

  /// Flag for changes in watched directory
  std::atomic_bool m_directoryChanged;

  /// Timer of running timer
  QTimer *m_timer;

  /// Last run loaded by auto
  int m_lastRunLoadedAuto;

  /// Files that are loaded
  std::vector<std::string> m_filesLoaded;

  /// Last run added by auto was addes as range
  std::atomic_bool m_wasLastAutoRange;

  /// Previous first run number (INSTNAMERUNNUMBER)
  std::string m_previousFirstRun;
};

} // namespace CustomInterfaces
} // namespace MantidQt

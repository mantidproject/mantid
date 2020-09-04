// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  void setData(const Mantid::API::MatrixWorkspace_sptr &data);

  // Returns a boolean stating whether data is currently being loading
  bool isLoading() const;

  // Cancels current loading algorithm
  void cancelLoading() const;

private slots:
  /// Check file range and call method to load new data
  void handleLoadRequested();

  /// Updates the list of logs and number of periods
  void updateAvailableInfo();

  /// Updates the run number found from auto in the view
  void updateAutoRun();

  void resetAutoRun();

signals:
  /// Signal emitted when data get changed
  void dataChanged();

private:
  /// Load new data and update the view accordingly
  void load(const std::vector<std::string> &files);

  void updateRunsTextFromAuto(const int autoRun);

  /// Check custom grouping is sensible
  bool checkCustomGrouping();

  /// Extract run number as in from file name string
  int extractRunNumber(const std::string &file);

  /// Check the group is valid
  std::string isCustomGroupingValid(const std::string &group, bool &isValid);

  /// View which the object works with
  IALCDataLoadingView *const m_view;

  /// Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr m_loadedData;

  /// Number of detectors for current first run
  size_t m_numDetectors;

  // bool to state whether loading data
  std::atomic_bool m_loadingData;

  // Loading algorithm
  Mantid::API::IAlgorithm_sptr m_LoadingAlg;

  /// Input entered before auto checked
  std::string m_oldInput;
};

} // namespace CustomInterfaces
} // namespace MantidQt

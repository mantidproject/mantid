// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IALCDataLoadingModel.h"
#include "IALCDataLoadingPresenter.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include <atomic>

namespace MantidQt {
namespace CustomInterfaces {
class IALCDataLoadingPresenterSubscriber;
class IALCDataLoadingView;

/** ALCDataLoadingPresenter : Presenter for ALC Data Loading step
 */
class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingPresenter : public IALCDataLoadingPresenter {

public:
  ALCDataLoadingPresenter(IALCDataLoadingView *view, std::unique_ptr<IALCDataLoadingModel> model);

  void initialize() override;

  void setSubscriber(IALCDataLoadingPresenterSubscriber *subscriber) override;

  /// @return Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr loadedData() const override { return m_model->getLoadedData(); }

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

  /// Handle for when runs have been searched for
  void handleRunsFound() override;

  /// When directory contents change, set flag
  void setDirectoryChanged(bool hasDirectoryChanged) override;

  /// Handle a user requests to see the period info widget
  void handlePeriodInfoClicked() override;

  /// Handle timer event that checks directory for new files added
  void handleTimerEvent() override;

  void handleWatcherStopped() override;

private:
  /// Load new data and update the view accordingly
  void loadFilesCurrentlyInModel();

  /// Update info on MuonPeriodInfo widget using sample logs from ws
  void updateAvailablePeriodInfo(const Mantid::API::MatrixWorkspace_sptr &ws);

  /// Subscriber to notify of loaded data changes.
  IALCDataLoadingPresenterSubscriber *m_subscriber;

  /// View which the object works with
  IALCDataLoadingView *m_view;

  // Model
  std::unique_ptr<IALCDataLoadingModel> m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

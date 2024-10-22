
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {

/** IALCDataLoadingPresenter : Abstract Presenter for ALC Data Loading step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingPresenter {

public:
  virtual ~IALCDataLoadingPresenter() = default;

  virtual void initialize() = 0;

  /// @return Last loaded data workspace
  virtual Mantid::API::MatrixWorkspace_sptr loadedData() const = 0;

  /// @return Loaded data as MatrixWorkspace_sptr
  virtual Mantid::API::MatrixWorkspace_sptr exportWorkspace() = 0;

  /// Sets some data
  virtual void setData(const Mantid::API::MatrixWorkspace_sptr &data) = 0;

  // Returns a boolean stating whether data is currently being loading
  virtual bool isLoading() const = 0;

  // Cancels current loading algorithm
  virtual void cancelLoading() const = 0;

  /// Check file range and call method to load new data
  virtual void handleLoadRequested() = 0;

  /// Updates the list of logs and number of periods
  virtual void updateAvailableInfo() = 0;

  /// Handle for when runs editing starts
  virtual void handleRunsEditing() = 0;

  /// Handle for when runs editing finishes
  virtual void handleRunsEditingFinished() = 0;

  /// Handle for when instrument changed
  virtual void handleInstrumentChanged(const std::string &instrument) = 0;

  /// Handle for when runs have been searched for
  virtual void handleRunsFound() = 0;

  /// When directory contents change, set flag
  virtual void updateDirectoryChangedFlag() = 0;

  /// Handle a user requests to see the period info widget
  virtual void handlePeriodInfoClicked() = 0;

  /// Handle timer event that checks directory for new files added
  virtual void handleTimerEvent() = 0;

  virtual void resetLatestAutoRunAndWasAutoRange() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

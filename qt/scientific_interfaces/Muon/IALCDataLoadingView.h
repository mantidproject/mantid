// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"
#include <QFileSystemWatcher>
#include <QTimer>

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"

#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
/** IALCDataLoadingView : View interface for ALC Data Loading step
 */
class IALCDataLoadingPresenter;

class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingView : public QObject {

public:
  virtual void subscribePresenter(IALCDataLoadingPresenter *presenter) = 0;
  /// Init instrument combo box
  virtual void initInstruments() = 0;

  /// @return std::string of instrument name
  virtual std::string getInstrument() const = 0;

  /// @return std::string of path
  virtual std::string getPath() const = 0;

  /// Returns the name of the log to use
  /// @return Log name
  virtual std::string log() const = 0;

  /// Returns the function to apply
  /// @return Log function
  virtual std::string function() const = 0;

  /// @return dead time correction type to use
  virtual std::string deadTimeType() const = 0;

  /// @return dead time correction file
  virtual std::string deadTimeFile() const = 0;

  /// @return detector grouping type
  virtual std::string detectorGroupingType() const = 0;

  /// @return forward grouping
  virtual std::string getForwardGrouping() const = 0;

  /// @return backward grouping
  virtual std::string getBackwardGrouping() const = 0;

  /// @return red period
  virtual std::string redPeriod() const = 0;

  /// @return green period
  virtual std::string greenPeriod() const = 0;

  /// @return subtract checkbox
  virtual bool subtractIsChecked() const = 0;

  /// @return Selected calculation type - "Integral" or "Differential"
  virtual std::string calculationType() const = 0;

  /// @return Selected integration time range, or nothing if limiting is
  /// disabled
  virtual std::optional<std::pair<double, double>> timeRange() const = 0;

  /// Sets all available info to empty
  virtual void setAvailableInfoToEmpty() = 0;

  /// Get text input for runs
  virtual std::string getRunsText() const = 0;

  /// Manual override of Auto add runs
  virtual void toggleRunsAutoAdd(const bool on) = 0;

  virtual std::string getRunsFirstRunText() const = 0;

  /// Enable/disable alpha to be editable
  virtual void enableAlpha(const bool alpha) = 0;

  virtual bool isAlphaEnabled() const = 0;

  /// Set value of alpha
  virtual void setAlphaValue(const std::string &alpha) = 0;

  /// Get alpha value
  virtual std::string getAlphaValue() const = 0;

  /// Show/hide alpha explanation
  virtual void showAlphaMessage(const bool alpha) = 0;

  virtual void setFileExtensions(const std::vector<std::string> &extensions) = 0;

  /// Period Info Widget displayed from the view
  virtual std::shared_ptr<MantidQt::MantidWidgets::MuonPeriodInfo> getPeriodInfo() = 0;

  virtual QFileSystemWatcher *getFileSystemWatcher() = 0;

  virtual QTimer *getTimer() = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /// Updates the plotted data
  /// @param workspace :: The workspace containing the data
  /// @param workspaceIndex :: the index to plot
  virtual void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /// Displays an error
  /// @param error :: Error message to display
  virtual void displayError(const std::string &error) = 0;

  /// Displays a warning to the user
  /// @param warning :: Warning message to display
  /// @return Users response
  virtual bool displayWarning(const std::string &warning) = 0;

  /// Update the list of logs user can select
  /// @param logs :: New list of log names
  virtual void setAvailableLogs(const std::vector<std::string> &logs) = 0;

  /// Update the list of periods user can select
  /// @param periods :: New list of periods
  virtual void setAvailablePeriods(const std::vector<std::string> &periods) = 0;

  /// Update the time limits
  /// @param tMin :: Minimum X value available
  /// @param tMax :: Maximum X value available
  virtual void setTimeLimits(double tMin, double tMax) = 0;

  /// Update the time limits
  /// @param tMin :: Minimum X value available
  /// @param tMax :: Maximum X value available
  virtual void setTimeRange(double tMin, double tMax) = 0;

  /// Opens the Mantid Wiki web page
  virtual void help() = 0;

  /// Disables all the widgets
  virtual void disableAll() = 0;

  /// Enables all the widgets
  virtual void enableAll() = 0;

  /// Enables/Disables the load button when ready
  virtual void enableLoad(bool enable) = 0;

  /// Sets path from where data loaded from
  virtual void setPath(const std::string &path) = 0;

  /// Sets the instrument in runs box if user changes it from combobox
  virtual void setInstrument(const std::string &instrument) = 0;

  /// Enables/Disables auto add
  virtual void enableRunsAutoAdd(bool enable) = 0;

  /// Get runs errors
  virtual std::string getRunsError() = 0;

  // Get files for loading
  virtual std::vector<std::string> getFiles() = 0;

  // Get first file only for loading
  virtual std::string getFirstFile() = 0;

  /// Set status label for loading
  virtual void setLoadStatus(const std::string &status, const std::string &colour) = 0;

  /// Handle check/uncheck of runs auto add
  virtual void runsAutoAddToggled(bool autoAdd) = 0;

  /// Sets text and ensure runs are not searched for
  virtual void setRunsTextWithoutSearch(const std::string &text) = 0;

  /// Slots for notifying presenter that view was changed
  virtual void instrumentChanged(QString instrument) = 0;

  virtual void notifyLoadClicked() = 0;

  virtual void notifyRunsEditingChanged() = 0;

  virtual void notifyRunsEditingFinished() = 0;

  virtual void notifyRunsFoundFinished() = 0;

  virtual void openManageDirectories() = 0;

  virtual void notifyPeriodInfoClicked() = 0;

  virtual void notifyTimerEvent() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

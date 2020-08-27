// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"

#include <QObject>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
/** IALCDataLoadingView : View interface for ALC Data Loading step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingView : public QObject {
  Q_OBJECT

public:
  /// @return Full path to first run data file, or empty string if invalid/not
  /// selected
  virtual std::string firstRun() const = 0;

  /// @return Full path to last run data file, or empty string if invalid/not
  /// selected
  virtual std::string lastRun() const = 0;

  /// @return Vector of file names
  virtual std::vector<std::string> getRuns() const = 0;

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
  virtual boost::optional<std::pair<double, double>> timeRange() const = 0;

  /// Sets the run number found from auto
  virtual void setCurrentAutoRun(const int run) = 0;

  /// @return text from ALC interface runs filefinder widget
  virtual std::string getCurrentRunsText() const = 0;

  /// Sets text of runs with a search for files
  virtual void setRunsTextWithSearch(const QString &text) = 0;

  /// @return string which was saved when auto checked
  virtual std::string getRunsOldInput() const = 0;

  /// Sets old input when auto checked
  virtual void setRunsOldInput(const std::string &oldInput) = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /// Updates the plotted data
  /// @param workspace :: The workspace containing the data
  /// @param workspaceIndex :: the index to plot
  virtual void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace,
                            std::size_t const &workspaceIndex = 0) = 0;

  /// Displays an error
  /// @param error :: Error message to display
  virtual void displayError(const std::string &error) = 0;

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

  /// Toggles "auto" mode for last file
  virtual void checkBoxAutoChanged(int state) = 0;

  /// Extract run number from a file
  virtual int extractRunNumber(const std::string &file) = 0;

signals:
  /// Request to load data
  void loadRequested();

  /// User has changed runs
  void runsSelected();

  /// New data have been loaded
  void dataChanged();

  /// "Auto" box has been checked
  void runAutoChecked();

  /// "Auto" box has been unchecked
  void runAutoUnchecked();
};

} // namespace CustomInterfaces
} // namespace MantidQt

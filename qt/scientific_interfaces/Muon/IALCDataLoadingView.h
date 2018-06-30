#ifndef MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include <QObject>
#include <boost/optional.hpp>
#include <qwt_data.h>

namespace MantidQt {
namespace CustomInterfaces {
/** IALCDataLoadingView : View interface for ALC Data Loading step

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
class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingView : public QObject {
  Q_OBJECT

public:
  /// @return Full path to first run data file, or empty string if invalid/not
  /// selected
  virtual std::string firstRun() const = 0;

  /// @return Full path to last run data file, or empty string if invalid/not
  /// selected
  virtual std::string lastRun() const = 0;

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

  /// @return The string "Auto"
  virtual std::string autoString() const = 0;

  /// If Auto mode on, store name of currently loaded file
  /// @param file :: [input] name of file loaded
  virtual void setCurrentAutoFile(const std::string &file) = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /// Updates the data curve
  /// @param data :: New curve data to set
  /// @param errors :: New curve errors to set
  virtual void setDataCurve(const QwtData &data,
                            const std::vector<double> &errors) = 0;

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

  /// Gets directory from first file and sets last file directory
  virtual void handleFirstFileChanged() = 0;

signals:
  /// Request to load data
  void loadRequested();

  /// User has selected the first run
  void firstRunSelected();

  /// New data have been loaded
  void dataChanged();

  /// "Auto" box has been checked/unchecked
  void lastRunAutoCheckedChanged(int state);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_ */

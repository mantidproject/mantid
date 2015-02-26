#ifndef MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidKernel/System.h"

#include <QObject>
#include <qwt_data.h>
#include <boost/optional.hpp>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IALCDataLoadingView : View interface for ALC Data Loading step

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCDataLoadingView : public QObject
  {
    Q_OBJECT

  public:
    /// @return Full path to first run data file, or empty string if invalid/not selected
    virtual std::string firstRun() const = 0;

    /// @return Full path to last run data file, or empty string if invalid/not selected
    virtual std::string lastRun() const = 0;

    /// Returns the name of the log to use
    /// @return Log name
    virtual std::string log() const = 0;

    /// @return dead time correction type to use
    virtual std::string deadTimeType() const = 0;

    /// @return dead time correction file
    virtual std::string deadTimeFile() const = 0;

    /// @return Selected calculation type - "Integral" or "Differential"
    virtual std::string calculationType() const = 0;

    /// @return Selected integration time range, or nothing if limiting is disabled
    virtual boost::optional< std::pair<double,double> > timeRange() const = 0;

  public slots:
    /// Performs any necessary initialization
    virtual void initialize() = 0;

    /// Updates the data curve
    /// @param data :: New curve data to set
    virtual void setDataCurve(const QwtData& data) = 0;

    /// Displays an error
    /// @param error :: Error message to display
    virtual void displayError(const std::string& error) = 0;

    /// Update the list of logs user can select
    /// @param logs :: New list of log names
    virtual void setAvailableLogs(const std::vector<std::string>& logs) = 0;

    /// Set waiting cursor for long operation
    virtual void setWaitingCursor() = 0;

    /// Restore the original cursor
    virtual void restoreCursor() = 0;

  signals:
    /// Request to load data
    void loadRequested();

    /// User has selected the first run
    void firstRunSelected();
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_IALCDATALOADINGVIEW_H_ */

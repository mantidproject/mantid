#ifndef MANTID_DATAHANDLING_ISISRUNLOGS_H_
#define MANTID_DATAHANDLING_ISISRUNLOGS_H_

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/LogParser.h"
#include "MantidKernel/System.h"

#include "MantidAPI/Run.h"

#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace DataHandling {

/**

  Defines a class to aid in creating ISIS specific run logs for periods, status
  etc.
  This adds:
    - status log: "running"
    - current period log: "period x"
    - all periods: "periods"

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ISISRunLogs {
public:
  /// Construct this object using a run that has the required ICP event log
  /// and the number of periods
  ISISRunLogs(const API::Run &icpRun, const int totalNumPeriods);

  /// Adds the status log to the this run
  void addStatusLog(API::Run &exptRun);
  /// Adds period related logs
  void addPeriodLogs(const int period, API::Run &exptRun);
  /// Add 'period i' log.
  void addPeriodLog(const int i, API::Run &exptRun);

private:
  DISABLE_DEFAULT_CONSTRUCT(ISISRunLogs)

  /// A LogParser object
  boost::scoped_ptr<Kernel::LogParser> m_logParser;
  /// The total number of periods in original data file
  const int m_numOfPeriods;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ISISRUNLOGS_H_ */

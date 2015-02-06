#ifndef MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUE_H_
#define MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**
  Produces a table or single spectrum workspace containing the total summed
  events in the workspace
  as a function of a specified log.

  Copyright &copy; 2012-3 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SumEventsByLogValue : public API::Algorithm {
public:
  SumEventsByLogValue();
  virtual ~SumEventsByLogValue();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SumEventsByLogValue"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Events"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Produces a single spectrum workspace containing the "
           "total summed events in the workspace as a function of a specified "
           "log.";
  }

  std::map<std::string, std::string> validateInputs();

private:
  void init();
  void exec();

  void createTableOutput(const Kernel::TimeSeriesProperty<int> *log);

  template <typename T>
  void createBinnedOutput(const Kernel::TimeSeriesProperty<T> *log);

  void filterEventList(const API::IEventList &eventList, const int minVal,
                       const int maxVal,
                       const Kernel::TimeSeriesProperty<int> *log,
                       std::vector<int> &Y);
  void addMonitorCounts(API::ITableWorkspace_sptr outputWorkspace,
                        const Kernel::TimeSeriesProperty<int> *log,
                        const int minVal, const int maxVal);
  std::vector<std::pair<std::string, const Kernel::ITimeSeriesProperty *>>
  getNumberSeriesLogs();
  double
  sumProtonCharge(const Kernel::TimeSeriesProperty<double> *protonChargeLog,
                  const Kernel::TimeSplitterType &filter);

  DataObjects::EventWorkspace_const_sptr
      m_inputWorkspace;                ///< The input workspace
  std::string m_logName;               ///< The name of the log to sum against
  std::vector<double> m_binningParams; ///< The optional binning parameters
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUE_H_ */

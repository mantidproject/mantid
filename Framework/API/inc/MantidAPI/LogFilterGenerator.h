#ifndef MANTID_API_LOGFILTERGENERATOR_H_
#define MANTID_API_LOGFILTERGENERATOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/Property.h"
#include <memory>

namespace Mantid {
namespace API {

/** LogFilterGenerator : utility to generate a LogFilter, to filter by running
  status or period

  This was refactored out of MantidUI::importNumSeriesLog

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL LogFilterGenerator {
public:
  /// Types of filter that can be used
  enum class FilterType { None, Status, Period, StatusAndPeriod };

  /// Constructor taking workspace
  LogFilterGenerator(const FilterType filterType,
                     const Mantid::API::MatrixWorkspace_const_sptr &workspace);

  /// Constructor taking run object
  LogFilterGenerator(const FilterType filterType, const Mantid::API::Run &run);

  /// Generate log filter from given workspace and log name
  std::unique_ptr<Mantid::Kernel::LogFilter>
  generateFilter(const std::string &logName) const;

private:
  /// Filter log by "running" status
  void filterByStatus(Mantid::Kernel::LogFilter *filter) const;
  /// Filter log by period
  void filterByPeriod(Mantid::Kernel::LogFilter *filter) const;
  /// Get log data from workspace
  Mantid::Kernel::Property *getLogData(const std::string &logName,
                                       bool warnIfNotFound = true) const;
  /// Type of filter
  const FilterType m_filterType;
  /// Run object containing logs
  const Mantid::API::Run m_run;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LOGFILTERGENERATOR_H_ */
#ifndef MANTID_DATAHANDLING_MERGELOGS_H_
#define MANTID_DATAHANDLING_MERGELOGS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DataHandling {

/** Merge2WorkspaceLogs : TODO: DESCRIPTION

  @date 2011-12-15

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Merge2WorkspaceLogs : public API::Algorithm {
public:
  Merge2WorkspaceLogs();
  virtual ~Merge2WorkspaceLogs();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "MergeLogs"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Merge 2 TimeSeries logs in a given Workspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling\\Logs"; }

private:
  void init();

  void exec();

  void mergeLogs(std::string ilogname1, std::string ilogname2,
                 std::string ologname, bool resetlogvalue, double logvalue1,
                 double logvalue2);

  Kernel::TimeSeriesProperty<double> *getTimeSeriesLog(std::string logname);

  API::MatrixWorkspace_sptr matrixWS;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_MERGELOGS_H_ */

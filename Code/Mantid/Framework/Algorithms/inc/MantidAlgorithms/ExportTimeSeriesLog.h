#ifndef MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_
#define MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/** ExportTimeSeriesLog : Read a TimeSeries log and return some information
  required by users.

  @date 2011-12-22

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
class DLLExport ExportTimeSeriesLog : public API::Algorithm {
public:
  ExportTimeSeriesLog();
  virtual ~ExportTimeSeriesLog();

  virtual const std::string name() const { return "ExportTimeSeriesLog"; };
  virtual int version() const { return 1; };
  virtual const std::string category() const {
    return "Diffraction;Events\\EventFiltering";
  };

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Read a TimeSeries log and return information";
  }

private:
  API::MatrixWorkspace_sptr m_dataWS;
  API::MatrixWorkspace_sptr m_outWS;

  std::vector<int64_t> mSETimes;
  std::vector<double> mSEValues;

  Kernel::DateAndTime mRunStartTime;
  Kernel::DateAndTime mFilterT0;
  Kernel::DateAndTime mFilterTf;

  void init();

  void exec();

  void exportLog(std::string logname, int numberexports, bool outputeventws);

  void setupEventWorkspace(int numentries,
                           std::vector<Kernel::DateAndTime> &times,
                           std::vector<double> values);

  void setupWorkspace2D(int numentries, std::vector<Kernel::DateAndTime> &times,
                        std::vector<double> values);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_ */

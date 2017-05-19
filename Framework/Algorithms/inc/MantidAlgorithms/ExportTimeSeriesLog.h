#ifndef MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_
#define MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
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
  const std::string name() const override { return "ExportTimeSeriesLog"; };

  int version() const override { return 1; };

  const std::string category() const override {
    return "Diffraction\\DataHandling;Events\\EventFiltering";
  };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Read a TimeSeries log and return information";
  }

private:
  API::MatrixWorkspace_sptr m_inputWS;
  API::MatrixWorkspace_sptr m_outWS;

  std::vector<int64_t> mSETimes;
  std::vector<double> mSEValues;

  Kernel::DateAndTime mRunStartTime;
  Kernel::DateAndTime mFilterT0;
  Kernel::DateAndTime mFilterTf;

  void init() override;

  void exec() override;

  bool
  calculateTimeSeriesRangeByTime(std::vector<Kernel::DateAndTime> &vec_times,
                                 const double &rel_start_time, size_t &i_start,
                                 const double &rel_stop_time, size_t &i_stop,
                                 const double &time_factor);

  void exportLog(const std::string &logname, const std::string timeunit,
                 const double &starttime, const double &stoptime,
                 const bool exportepoch, bool outputeventws, int numentries,
                 bool cal_first_deriv);

  void setupEventWorkspace(const size_t &start_index, const size_t &stop_index,
                           int numentries,
                           std::vector<Kernel::DateAndTime> &times,
                           std::vector<double> values, const bool &epochtime);

  void setupWorkspace2D(const size_t &start_index, const size_t &stop_index,
                        int numentries, std::vector<Kernel::DateAndTime> &times,
                        std::vector<double> values, const bool &epochtime,
                        const double &timeunitfactor, size_t nspec);

  void calculateFirstDerivative(bool is_event_ws);

  void setupMetaData(const std::string &log_name, const std::string &time_unit,
                     const bool &export_epoch);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXPORTTIMESERIESLOG_H_ */

#ifndef MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_
#define MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** GetTimeSeriesLogInformation : Read a TimeSeries log and return some
  information required by users.

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
class DLLExport GetTimeSeriesLogInformation : public API::Algorithm {
public:
  GetTimeSeriesLogInformation();

  const std::string name() const override {
    return "GetTimeSeriesLogInformation";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Get information from a TimeSeriesProperty log.";
  }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLogMultiple"};
  }
  const std::string category() const override {
    return "Diffraction\\Utility;Events\\EventFiltering";
  }

private:
  API::MatrixWorkspace_sptr m_dataWS;

  Types::Core::DateAndTime mRunStartTime;
  Types::Core::DateAndTime mFilterT0;
  Types::Core::DateAndTime mFilterTf;

  std::map<std::string, std::size_t> m_intInfoMap;
  std::map<std::string, double> m_dblInfoMap;

  Kernel::TimeSeriesProperty<double> *m_log;
  std::vector<Types::Core::DateAndTime> m_timeVec;
  std::vector<double> m_valueVec;

  Types::Core::DateAndTime m_starttime;
  Types::Core::DateAndTime m_endtime;

  bool m_ignoreNegativeTime;

  void init() override;

  void exec() override;

  void examLog(std::string logname, std::string outputdir);

  void generateCalibrationFile();

  void processTimeRange();

  /// Calcualte the distribution of delta T in time stamps
  DataObjects::Workspace2D_sptr
  calDistributions(std::vector<Types::Core::DateAndTime> timevec,
                   double stepsize);

  void exportLog(API::MatrixWorkspace_sptr ws,
                 std::vector<Types::Core::DateAndTime> abstimevec, double dts);

  void setupEventWorkspace(int numentries,
                           std::vector<Types::Core::DateAndTime> &times,
                           std::vector<double> values);

  void setupWorkspace2D(int numentries,
                        std::vector<Types::Core::DateAndTime> &times,
                        std::vector<double> values);

  void execQuickStatistics();

  void exportErrorLog(API::MatrixWorkspace_sptr ws,
                      std::vector<Types::Core::DateAndTime> abstimevec,
                      double dts);

  void checkLogValueChanging(std::vector<Types::Core::DateAndTime> timevec,
                             std::vector<double> values, double delta);

  void checkLogBasicInforamtion();

  /// Generate statistic information table workspace
  DataObjects::TableWorkspace_sptr generateStatisticTable();

  Types::Core::DateAndTime getAbsoluteTime(double abstimens);

  Types::Core::DateAndTime calculateRelativeTime(double deltatime);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_ */

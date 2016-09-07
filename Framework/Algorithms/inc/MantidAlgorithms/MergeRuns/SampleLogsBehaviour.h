#ifndef MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_
#define MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_

#include "MantidAlgorithms/DllConfig.h"
#include <MantidAPI/MatrixWorkspace.h>

namespace Mantid {
namespace Algorithms {

/** MergeRunsSampleLogsBehaviour : This class holds information relating to the
  behaviour of the sample log merging. It holds a map of all the sample log
  parameters to merge, how to merge them, and the associated tolerance.

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
class MANTID_ALGORITHMS_DLL SampleLogsBehaviour {
public:
  enum MergeLogType { time_series, list, warn, fail };

  static const std::string TIME_SERIES_MERGE;
  static const std::string LIST_MERGE;
  static const std::string WARN_MERGE;
  static const std::string FAIL_MERGE;
  static const std::string WARN_MERGE_TOLERANCES;
  static const std::string FAIL_MERGE_TOLERANCES;

  static const std::string TIME_SERIES_SUFFIX;
  static const std::string LIST_SUFFIX;

  typedef struct {
    MergeLogType type;
    std::shared_ptr<Kernel::Property> property;
    double tolerance;
    bool isNumeric;
  } SampleLogBehaviour;

  SampleLogsBehaviour(const API::MatrixWorkspace_sptr &ws,
                      Kernel::Logger &logger,
                      const std::string sampleLogsTimeSeries,
                      const std::string sampleLogsList,
                      const std::string sampleLogsWarn,
                      const std::string sampleLogsWarnTolerances,
                      const std::string sampleLogsFail,
                      const std::string sampleLogsFailTolerances);

  Kernel::Logger &m_logger;

  typedef std::map<const std::string, SampleLogBehaviour> SampleLogsMap;
  SampleLogsMap m_logMap;

  /// Create and update sample logs according to instrument parameters
  void
  createSampleLogsMapsFromInstrumentParams(SampleLogsMap &instrumentMap,
                                           const API::MatrixWorkspace_sptr &ws);
  void calculateUpdatedSampleLogs(const API::MatrixWorkspace_sptr &ws,
                                  const API::MatrixWorkspace_sptr &outWS);
  void setUpdatedSampleLogs(const API::MatrixWorkspace_sptr &ws);
  void resetSampleLogs(const API::MatrixWorkspace_sptr &ws);

private:
  void updateSampleMap(SampleLogsMap &map, const MergeLogType &,
                       const std::string &params,
                       const API::MatrixWorkspace_sptr &ws,
                       const std::string paramsTolerances = "",
                       const bool skipIfInPrimaryMap = false);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_ */

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_
#define MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_

#include "MantidAlgorithms/DllConfig.h"
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidKernel/Logger.h>
#include <MantidKernel/Property.h>

namespace Mantid {
namespace Algorithms {

/** SampleLogsBehaviour : This class holds information relating to the
  behaviour of the sample log merging. It holds a map of all the sample log
  parameters to merge, how to merge them, and the associated tolerances.
*/
class MANTID_ALGORITHMS_DLL SampleLogsBehaviour {
public:
  enum class MergeLogType { Sum, TimeSeries, List, Warn, Fail };

  const std::string SUM_MERGE;
  const std::string TIME_SERIES_MERGE;
  const std::string LIST_MERGE;
  const std::string WARN_MERGE;
  const std::string WARN_MERGE_TOLERANCES;
  const std::string FAIL_MERGE;
  const std::string FAIL_MERGE_TOLERANCES;

  // the names and docs of the override properties
  static const std::string TIME_SERIES_PROP;
  static const std::string TIME_SERIES_DOC;
  static const std::string LIST_PROP;
  static const std::string LIST_DOC;
  static const std::string WARN_PROP;
  static const std::string WARN_DOC;
  static const std::string WARN_TOL_PROP;
  static const std::string WARN_TOL_DOC;
  static const std::string FAIL_PROP;
  static const std::string FAIL_DOC;
  static const std::string FAIL_TOL_PROP;
  static const std::string FAIL_TOL_DOC;
  static const std::string SUM_PROP;
  static const std::string SUM_DOC;

  struct SampleLogBehaviour {
    std::shared_ptr<Kernel::Property> property;
    double tolerance;
    bool isNumeric;
  };

  SampleLogsBehaviour(
      API::MatrixWorkspace_sptr &ws, Kernel::Logger &logger,
      const std::string &sampleLogsSum = "",
      const std::string &sampleLogsTimeSeries = "",
      const std::string &sampleLogsList = "",
      const std::string &sampleLogsWarn = "",
      const std::string &sampleLogsWarnTolerances = "",
      const std::string &sampleLogsFail = "",
      const std::string &sampleLogsFailTolerances = "",
      const std::string &sum_merge = "sample_logs_sum",
      const std::string &time_series_merge = "sample_logs_time_series",
      const std::string &list_merge = "sample_logs_list",
      const std::string &warn_merge = "sample_logs_warn",
      const std::string &warn_merge_tolerances = "sample_logs_warn_tolerances",
      const std::string &fail_merge = "sample_logs_fail",
      const std::string &fail_merge_tolerances = "sample_logs_fail_tolerances");

  /// Create and update sample logs according to instrument parameters
  void mergeSampleLogs(API::MatrixWorkspace_sptr &addeeWS,
                       API::MatrixWorkspace_sptr &outWS);
  void setUpdatedSampleLogs(API::MatrixWorkspace_sptr &outWS);
  void removeSampleLogsFromWorkspace(API::MatrixWorkspace_sptr &addeeWS);
  void readdSampleLogToWorkspace(API::MatrixWorkspace_sptr &addeeWS);
  void resetSampleLogs(API::MatrixWorkspace_sptr &ws);

private:
  Kernel::Logger &m_logger;

  using SampleLogsKey = std::pair<std::string, MergeLogType>;
  using SampleLogsMap = std::map<SampleLogsKey, SampleLogBehaviour>;
  SampleLogsMap m_logMap;
  std::vector<std::shared_ptr<Kernel::Property>> m_addeeLogMap;

  void createSampleLogsMapsFromInstrumentParams(SampleLogsMap &instrumentMap,
                                                API::MatrixWorkspace_sptr &ws);

  std::shared_ptr<Kernel::Property>
  addPropertyForTimeSeries(const std::string &item, const double value,
                           API::MatrixWorkspace_sptr &ws);
  std::shared_ptr<Kernel::Property>
  addPropertyForList(const std::string &item, const std::string &value,
                     API::MatrixWorkspace_sptr &ws);
  bool setNumericValue(const std::string &item, const API::MatrixWorkspace_sptr &ws,
                       double &value);

  void setSampleMap(SampleLogsMap &map, const MergeLogType &,
                    const std::string &params, API::MatrixWorkspace_sptr &ws,
                    const std::string &paramsTolerances = "",
                    bool skipIfInPrimaryMap = false);

  std::vector<double>
  createTolerancesVector(const size_t numberNames,
                         const std::vector<std::string> &tolerances);

  void updateSumProperty(double addeeWSNumber, double outWSNumber,
                         API::MatrixWorkspace_sptr &outWS, const std::string &name);
  void updateTimeSeriesProperty(API::MatrixWorkspace_sptr &addeeWS,
                                API::MatrixWorkspace_sptr &outWS,
                                const std::string &name);
  void updateListProperty(API::MatrixWorkspace_sptr &addeeWS,
                          API::MatrixWorkspace_sptr &outWS, const std::string &name);
  void checkWarnProperty(const API::MatrixWorkspace_sptr &addeeWS,
                         Kernel::Property *addeeWSProperty,
                         const SampleLogBehaviour &behaviour,
                         const double addeeWSNumber, const double outWSNumber,
                         const std::string &name);
  void checkErrorProperty(const API::MatrixWorkspace_sptr &addeeWS,
                          Kernel::Property *addeeWSProperty,
                          const SampleLogBehaviour &behaviour,
                          const double addeeWSNumber, const double outWSNumber,
                          const std::string &name);

  bool isWithinTolerance(const SampleLogBehaviour &behaviour,
                         const double addeeWSNumber, const double outWSNumber);
  bool stringPropertiesMatch(const SampleLogBehaviour &behaviour,
                             const Kernel::Property *addeeWSProperty);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_ */

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"

namespace Mantid {
namespace Algorithms {

/** SampleLogsBehaviour : This class holds information relating to the
  behaviour of the sample log merging. It holds a map of all the sample log
  parameters to merge, how to merge them, and the associated tolerances.
  Algorithms which already define paramter names for the instrument parameter
  file are ConjoinXRuns and MergeRuns. Please use different names for new
  algorithms.
*/
class MANTID_ALGORITHMS_DLL SampleLogsBehaviour {
public:
  enum class MergeLogType { Sum, TimeSeries, List, Warn, Fail };

  // names of parameters in IPF containing names of sample log entries as values
  struct ParameterName {
    std::string SUM_MERGE;
    std::string TIME_SERIES_MERGE;
    std::string LIST_MERGE;
    std::string WARN_MERGE;
    std::string WARN_MERGE_TOLERANCES;
    std::string FAIL_MERGE;
    std::string FAIL_MERGE_TOLERANCES;
  } parameterNames;

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

  // override sample log entries for specific merge type
  struct SampleLogNames {
    std::string sampleLogsSum;
    std::string sampleLogsTimeSeries;
    std::string sampleLogsList;
    std::string sampleLogsWarn;
    std::string sampleLogsWarnTolerances;
    std::string sampleLogsFail;
    std::string sampleLogsFailTolerances;
  };

  SampleLogsBehaviour(const API::MatrixWorkspace_sptr &ws, Kernel::Logger &logger,
                      const SampleLogNames &logEntries = {}, ParameterName parName = {});

  /// Create and update sample logs according to instrument parameters
  void mergeSampleLogs(const API::MatrixWorkspace_sptr &addeeWS, const API::MatrixWorkspace_sptr &outWS);
  void setUpdatedSampleLogs(const API::MatrixWorkspace_sptr &outWS);
  void removeSampleLogsFromWorkspace(const API::MatrixWorkspace_sptr &addeeWS);
  void readdSampleLogToWorkspace(const API::MatrixWorkspace_sptr &addeeWS);
  void resetSampleLogs(const API::MatrixWorkspace_sptr &ws);

private:
  Kernel::Logger &m_logger;

  using SampleLogsKey = std::pair<std::string, MergeLogType>;
  using SampleLogsMap = std::map<SampleLogsKey, SampleLogBehaviour>;
  SampleLogsMap m_logMap;
  std::vector<std::shared_ptr<Kernel::Property>> m_addeeLogMap;

  void createSampleLogsMapsFromInstrumentParams(SampleLogsMap &instrumentMap, API::MatrixWorkspace &ws);

  std::shared_ptr<Kernel::Property> addPropertyForTimeSeries(const std::string &item, const double value,
                                                             API::MatrixWorkspace &ws);
  std::shared_ptr<Kernel::Property> addPropertyForList(const std::string &item, const std::string &value,
                                                       API::MatrixWorkspace &ws);
  bool setNumericValue(const std::string &item, const API::MatrixWorkspace &ws, double &value);

  void setSampleMap(SampleLogsMap &map, const MergeLogType &, const std::string &params, API::MatrixWorkspace &ws,
                    const std::string &paramsTolerances = "", bool skipIfInPrimaryMap = false);

  std::vector<double> createTolerancesVector(const size_t numberNames, const std::vector<std::string> &tolerances);

  void updateSumProperty(double addeeWSNumber, double outWSNumber, API::MatrixWorkspace &outWS,
                         const std::string &name);
  void updateTimeSeriesProperty(const API::MatrixWorkspace &addeeWS, const API::MatrixWorkspace &outWS,
                                const std::string &name);
  void updateListProperty(const API::MatrixWorkspace &addeeWS, API::MatrixWorkspace &outWS, const std::string &name);
  void checkWarnProperty(const API::MatrixWorkspace &addeeWS, const Kernel::Property *addeeWSProperty,
                         const SampleLogBehaviour &behaviour, const double addeeWSNumber, const double outWSNumber,
                         const std::string &name);
  void checkErrorProperty(const API::MatrixWorkspace &addeeWS, const Kernel::Property *addeeWSProperty,
                          const SampleLogBehaviour &behaviour, const double addeeWSNumber, const double outWSNumber,
                          const std::string &name);

  bool isWithinTolerance(const SampleLogBehaviour &behaviour, const double addeeWSNumber, const double outWSNumber);
  bool stringPropertiesMatch(const SampleLogBehaviour &behaviour, const Kernel::Property *addeeWSProperty);
};

} // namespace Algorithms
} // namespace Mantid

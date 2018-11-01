// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_
#define MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOUR_H_

#include "MantidAlgorithms/DllConfig.h"
#include <MantidAPI/MatrixWorkspace.h>

namespace Mantid {
namespace Algorithms {

/** SampleLogsBehaviour : This class holds information relating to the
  behaviour of the sample log merging. It holds a map of all the sample log
  parameters to merge, how to merge them, and the associated tolerances.
*/
class MANTID_ALGORITHMS_DLL SampleLogsBehaviour {
public:
  enum class MergeLogType { Sum, TimeSeries, List, Warn, Fail };

  static const std::string SUM_MERGE;
  static const std::string TIME_SERIES_MERGE;
  static const std::string LIST_MERGE;
  static const std::string WARN_MERGE;
  static const std::string FAIL_MERGE;
  static const std::string WARN_MERGE_TOLERANCES;
  static const std::string FAIL_MERGE_TOLERANCES;

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

  SampleLogsBehaviour(API::MatrixWorkspace &ws, Kernel::Logger &logger,
                      const std::string &sampleLogsSum = "",
                      const std::string &sampleLogsTimeSeries = "",
                      const std::string &sampleLogsList = "",
                      const std::string &sampleLogsWarn = "",
                      const std::string &sampleLogsWarnTolerances = "",
                      const std::string &sampleLogsFail = "",
                      const std::string &sampleLogsFailTolerances = "");

  /// Create and update sample logs according to instrument parameters
  void mergeSampleLogs(API::MatrixWorkspace &addeeWS,
                       API::MatrixWorkspace &outWS);
  void setUpdatedSampleLogs(API::MatrixWorkspace &outWS);
  void removeSampleLogsFromWorkspace(API::MatrixWorkspace &addeeWS);
  void readdSampleLogToWorkspace(API::MatrixWorkspace &addeeWS);
  void resetSampleLogs(API::MatrixWorkspace &ws);

private:
  Kernel::Logger &m_logger;

  using SampleLogsKey = std::pair<std::string, MergeLogType>;
  using SampleLogsMap = std::map<SampleLogsKey, SampleLogBehaviour>;
  SampleLogsMap m_logMap;
  std::vector<std::shared_ptr<Kernel::Property>> m_addeeLogMap;

  void createSampleLogsMapsFromInstrumentParams(SampleLogsMap &instrumentMap,
                                                API::MatrixWorkspace &ws);

  std::shared_ptr<Kernel::Property>
  addPropertyForTimeSeries(const std::string &item, const double value,
                           API::MatrixWorkspace &ws);
  std::shared_ptr<Kernel::Property>
  addPropertyForList(const std::string &item, const std::string &value,
                     API::MatrixWorkspace &ws);
  bool setNumericValue(const std::string &item, const API::MatrixWorkspace &ws,
                       double &value);

  void setSampleMap(SampleLogsMap &map, const MergeLogType &,
                    const std::string &params, API::MatrixWorkspace &ws,
                    const std::string &paramsTolerances = "",
                    bool skipIfInPrimaryMap = false);

  std::vector<double>
  createTolerancesVector(const size_t numberNames,
                         const std::vector<std::string> &tolerances);

  void updateSumProperty(double addeeWSNumber, double outWSNumber,
                         API::MatrixWorkspace &outWS, const std::string &name);
  void updateTimeSeriesProperty(API::MatrixWorkspace &addeeWS,
                                API::MatrixWorkspace &outWS,
                                const std::string &name);
  void updateListProperty(API::MatrixWorkspace &addeeWS,
                          API::MatrixWorkspace &outWS, const std::string &name);
  void checkWarnProperty(const API::MatrixWorkspace &addeeWS,
                         Kernel::Property *addeeWSProperty,
                         const SampleLogBehaviour &behaviour,
                         const double addeeWSNumber, const double outWSNumber,
                         const std::string &name);
  void checkErrorProperty(const API::MatrixWorkspace &addeeWS,
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

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_
#define MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

/** GenerateEventsFilter : Generate an events-filter, i.e., a SplittersWorkspace
  according
    to user's request.

    Request can be a combination of log value and time, i.e.,

    (1) T_min
    (2) T_max
    (3) delta Time
    (4) Min log value
    (5) Max log value
    (6) delta log value
    (7) identify log value increment (bool)
    (8) number of sections per interval (applied to log value only!)

    This algorithm can generate filters including
    (1) deltaT per interval from T_min to T_max with : Log value is not given
    (2) delta log value per interval from T_min to T_max and from min log value
  to max log value

    Note:
    (1) Time can be (a) relative time in ns  (b) relative time in second (float)
  (c) percentage time
    (2) if option "identify log value increment"

  @date 2012-04-09
*/
class DLLExport GenerateEventsFilter : public API::Algorithm {
public:
  explicit GenerateEventsFilter();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GenerateEventsFilter"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generate one or a set of event filters according to time or "
           "specified log's value.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"FilterEvents", "FilterByTime", "FilterByLogValue"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Events\\EventFiltering";
  }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Process properties
  void processInOutWorkspaces();

  void processInputTime();
  void setFilterByTimeOnly();
  void setFilterByLogValue(std::string logname);

  void processSingleValueFilter(double minvalue, double maxvalue,
                                bool filterincrease, bool filterdecrease);

  void processMultipleValueFilters(double minvalue, double valueinterval,
                                   double maxvalue, bool filterincrease,
                                   bool filterdecrease);

  void makeFilterBySingleValue(double min, double max, double TimeTolerance,
                               bool centre, bool filterIncrease,
                               bool filterDecrease,
                               Types::Core::DateAndTime startTime,
                               Types::Core::DateAndTime stopTime, int wsindex);

  /// Make multiple-log-value filters in serial
  void makeMultipleFiltersByValues(std::map<size_t, int> indexwsindexmap,
                                   std::vector<double> logvalueranges,
                                   bool centre, bool filterIncrease,
                                   bool filterDecrease,
                                   Types::Core::DateAndTime startTime,
                                   Types::Core::DateAndTime stopTime);

  /// Make multiple-log-value filters in serial in parallel
  void makeMultipleFiltersByValuesParallel(
      std::map<size_t, int> indexwsindexmap, std::vector<double> logvalueranges,
      bool centre, bool filterIncrease, bool filterDecrease,
      Types::Core::DateAndTime startTime, Types::Core::DateAndTime stopTime);

  /// Generate event splitters for partial sample log (serial)
  void makeMultipleFiltersByValuesPartialLog(
      int istart, int iend, std::vector<Types::Core::DateAndTime> &vecSplitTime,
      std::vector<int> &vecSplitGroup, std::map<size_t, int> indexwsindexmap,
      const std::vector<double> &logvalueranges, Types::Core::time_duration tol,
      bool filterIncrease, bool filterDecrease,
      Types::Core::DateAndTime startTime, Types::Core::DateAndTime stopTime);

  /// Generate event filters for integer sample log
  void processIntegerValueFilter(int minvalue, int maxvalue,
                                 bool filterIncrease, bool filterDecrease,
                                 Types::Core::DateAndTime runend);

  /// Search a value in a sorted vector
  size_t searchValue(const std::vector<double> &sorteddata, double value);

  /// Add a splitter
  void addNewTimeFilterSplitter(Types::Core::DateAndTime starttime,
                                Types::Core::DateAndTime stoptime, int wsindex,
                                std::string info);

  /// Create a splitter and add to the vector of time splitters
  Types::Core::DateAndTime
  makeSplitterInVector(std::vector<Types::Core::DateAndTime> &vecSplitTime,
                       std::vector<int> &vecGroupIndex,
                       Types::Core::DateAndTime start,
                       Types::Core::DateAndTime stop, int group, int64_t tol_ns,
                       Types::Core::DateAndTime lasttime);

  /// Generate a matrix workspace containing splitters
  void generateSplittersInMatrixWorkspace();

  /// Generate a matrix workspace from the parallel version
  void generateSplittersInMatrixWorkspaceParallel();

  /// Generate a SplittersWorkspace for filtering by log values
  void generateSplittersInSplitterWS();

  /// Identify the a sample log entry is within intended value and time region
  bool identifyLogEntry(const int &index, const Types::Core::DateAndTime &currT,
                        const bool &lastgood, const double &minvalue,
                        const double &maxvalue,
                        const Types::Core::DateAndTime &startT,
                        const Types::Core::DateAndTime &stopT,
                        const bool &filterIncrease, const bool &filterDecrease);

  /// Determine the chaning direction of log value
  int determineChangingDirection(int startindex);

  /// Find the end of the run
  Types::Core::DateAndTime findRunEnd();

  API::MatrixWorkspace_const_sptr m_dataWS;

  /// SplitterWorkspace
  DataObjects::SplittersWorkspace_sptr m_splitWS;
  /// Matrix workspace containing splitters
  API::MatrixWorkspace_sptr m_filterWS;

  API::ITableWorkspace_sptr m_filterInfoWS;

  Types::Core::DateAndTime m_startTime;
  Types::Core::DateAndTime m_stopTime;

  /// Run end time
  Types::Core::DateAndTime m_runEndTime;

  double m_timeUnitConvertFactorToNS;

  Kernel::TimeSeriesProperty<double> *m_dblLog;
  Kernel::TimeSeriesProperty<int> *m_intLog;

  bool m_logAtCentre;
  double m_logTimeTolerance;

  /// Flag to output matrix workspace for fast log
  bool m_forFastLog;

  /// SplitterType
  Kernel::TimeSplitterType m_splitters;
  /// Vector as date and time
  std::vector<Types::Core::DateAndTime> m_vecSplitterTime;
  std::vector<int> m_vecSplitterGroup;

  /// Processing algorithm type
  bool m_useParallel;

  std::vector<std::vector<Types::Core::DateAndTime>> m_vecSplitterTimeSet;
  std::vector<std::vector<int>> m_vecGroupIndexSet;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_ */

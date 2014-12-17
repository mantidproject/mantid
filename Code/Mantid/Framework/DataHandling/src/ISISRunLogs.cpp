// Includes
#include "MantidDataHandling/ISISRunLogs.h"

#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DataHandling {
namespace {
/// static logger
Kernel::Logger g_log("ISISRunLogs");
}

using Kernel::LogFilter;
using Kernel::LogParser;
using Kernel::TimeSeriesProperty;

/**
 * Construct using a run that has the required ICP event log
 * Throws if no icp event log exists
 * @param icpRun :: A run containing the ICP event log to parse
 * @param totalNumPeriods :: The total number of periods overall
 */
ISISRunLogs::ISISRunLogs(const API::Run &icpRun, const int totalNumPeriods)
    : m_logParser(), m_numOfPeriods(totalNumPeriods) {
  // ICP event either in form icp_event or icpevent
  static const char *icpLogNames[2] = {"icp_event", "icpevent"};
  for (int i = 0; i < 2; ++i) {
    try {
      Kernel::Property *icpLog = icpRun.getLogData(icpLogNames[i]);
      m_logParser.reset(new LogParser(icpLog));
      return;
    } catch (std::runtime_error &) {
    }
  }
  // If it does not exist then pass in a NULL log to indicate that period 1
  // should be assumed
  m_logParser.reset(new LogParser(NULL));
}

/**
 * Adds the status log to the this run.
 * @param exptRun :: The run that
 */
void ISISRunLogs::addStatusLog(API::Run &exptRun) {
  exptRun.addLogData(m_logParser->createRunningLog());
}

/**
 * Adds period related logs
 * @param period :: The period that we are adding to
 * @param exptRun :: The run for this period
 */
void ISISRunLogs::addPeriodLogs(const int period, API::Run &exptRun) {
  auto periodLog = m_logParser->createPeriodLog(period);
  LogFilter *logFilter(NULL);
  const TimeSeriesProperty<bool> *maskProp(NULL);
  try {
    auto runningLog =
        exptRun.getTimeSeriesProperty<bool>(LogParser::statusLogName());
    logFilter = new LogFilter(runningLog);
  } catch (std::exception &) {
    g_log.warning(
        "Cannot find status log. Logs will be not be filtered by run status");
  }

  // If there is more than 1 period filter the logs by period as well
  if (m_logParser->nPeriods() > 1) {
    if (logFilter) {
      logFilter->addFilter(*periodLog);
      maskProp = logFilter->filter();
    } else
      maskProp = periodLog;
  }
  // Filter logs if we have anything to filter on
  if (maskProp)
    exptRun.filterByLog(*maskProp);
  delete logFilter;

  exptRun.addProperty(periodLog);
  exptRun.addProperty(m_logParser->createCurrentPeriodLog(period));
  try {
    exptRun.addLogData(m_logParser->createAllPeriodsLog());
  } catch (std::runtime_error &) {
    // Already has one
  }
}

} // namespace DataHandling
} // namespace Mantid

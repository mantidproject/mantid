// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
#include "MantidDataHandling/ISISRunLogs.h"

#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DataHandling {
namespace {
/// static logger
Kernel::Logger g_log("ISISRunLogs");
} // namespace

using Kernel::LogFilter;
using Kernel::LogParser;
using Kernel::TimeSeriesProperty;

/**
 * Construct using a run that has the required ICP event log
 * Throws if no icp event log exists
 * @param icpRun :: A run containing the ICP event log to parse
 */
ISISRunLogs::ISISRunLogs(const API::Run &icpRun) {
  // ICP event either in form icp_event or icpevent
  for (const auto icpLogName : {"icp_event", "icpevent"}) {
    try {
      Kernel::Property *icpLog = icpRun.getLogData(icpLogName);
      m_logParser = std::make_unique<LogParser>(icpLog);
      return;
    } catch (std::runtime_error &) {
    }
  }
  // If it does not exist then pass in a NULL log to indicate that period 1
  // should be assumed
  m_logParser = std::make_unique<LogParser>(nullptr);
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
  auto logFilter = std::unique_ptr<LogFilter>();
  const TimeSeriesProperty<bool> *maskProp(nullptr);
  try {
    auto runningLog =
        exptRun.getTimeSeriesProperty<bool>(LogParser::statusLogName());
    logFilter = std::make_unique<LogFilter>(runningLog);
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

  exptRun.addProperty(periodLog);
  exptRun.addProperty(m_logParser->createCurrentPeriodLog(period));
  try {
    exptRun.addLogData(m_logParser->createAllPeriodsLog());
  } catch (std::runtime_error &) {
    // Already has one
  }
}

/**
 * Add the period log to a run.
 * @param period :: A period number.
 * @param exptRun :: The run to add the log to.
 */
void ISISRunLogs::addPeriodLog(const int period, API::Run &exptRun) {
  exptRun.addLogData(m_logParser->createPeriodLog(period));
}

} // namespace DataHandling
} // namespace Mantid

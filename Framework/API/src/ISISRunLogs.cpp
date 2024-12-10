// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
#include "MantidAPI/ISISRunLogs.h"

#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid::API {
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
void ISISRunLogs::addStatusLog(API::Run &exptRun) { exptRun.addLogData(m_logParser->createRunningLog()); }

/**
 * Adds period related logs, and applies log filtering
 * @param period :: The period that we are adding to
 * @param exptRun :: The run for this period
 */
void ISISRunLogs::addPeriodLogs(const int period, API::Run &exptRun) {
  auto periodLog = m_logParser->createPeriodLog(period);

  exptRun.addProperty(periodLog);
  exptRun.addProperty(m_logParser->createCurrentPeriodLog(period));
  try {
    exptRun.addLogData(m_logParser->createAllPeriodsLog());
  } catch (const std::runtime_error &) {
    // Already has one
  }

  ISISRunLogs::applyLogFiltering(exptRun);
}

/**
 * Applies log filtering buy run staus and period if available
 * @param exptRun :: The run for this period
 */
void ISISRunLogs::applyLogFiltering(Mantid::API::Run &exptRun) {
  std::unique_ptr<LogFilter> logFilter{nullptr};
  try {
    auto runningLog = exptRun.getTimeSeriesProperty<bool>(LogParser::statusLogName());
    logFilter = std::make_unique<LogFilter>(*runningLog);
  } catch (std::exception &) {
    g_log.warning("Cannot find status log. Logs will be not be filtered by run status");
  }

  TimeSeriesProperty<bool> *currentPeriodLog = nullptr;
  bool multiperiod = false;
  try {
    auto period = exptRun.getPropertyAsIntegerValue(LogParser::currentPeriodLogName());
    currentPeriodLog = exptRun.getTimeSeriesProperty<bool>(LogParser::currentPeriodLogName(period));
  } catch (const std::exception &) {
    g_log.warning("Cannot find period log. Logs will be not be filtered by "
                  "current period");
  }

  try {
    // get the number of periods as the max of the periods log
    auto periodsLog = exptRun.getTimeSeriesProperty<int>(LogParser::periodsLogName());
    multiperiod = (periodsLog->getStatistics().maximum > 1.);
  } catch (const std::exception &) {
    g_log.warning("Cannot find periods log. Logs will be not be filtered by "
                  "current period");
  }

  // If there is more than 1 period filter the logs by period as well
  if (multiperiod) {
    if (logFilter) {
      logFilter->addFilter(*currentPeriodLog);
    } else if (currentPeriodLog) {
      logFilter = std::make_unique<LogFilter>(*currentPeriodLog);
    }
  }

  // Filter logs if we have anything to filter on
  if (logFilter) {
    exptRun.filterByLog(logFilter.get(), ISISRunLogs::getLogNamesExcludedFromFiltering(exptRun));
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

std::vector<std::string> ISISRunLogs::getLogNamesExcludedFromFiltering(const API::Run &run) {
  std::vector<std::string> retVal;
  if (run.hasProperty(LogParser::statusLogName())) {
    retVal.emplace_back(LogParser::statusLogName());
  }
  if (run.hasProperty(LogParser::periodsLogName())) {
    retVal.emplace_back(LogParser::periodsLogName());
  }
  const auto &props = run.getProperties();
  for (const auto prop : props) {
    // add all properties starting with period
    if (prop->name().rfind("period", 0) == 0) {
      // add if not already in the list
      if (std::find(retVal.cbegin(), retVal.cend(), prop->name()) == retVal.cend()) {
        retVal.emplace_back(prop->name());
      }
    }
  }
  return retVal;
}

} // namespace Mantid::API

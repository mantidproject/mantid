// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/LogFilterGenerator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSeriesProperty.h"


using Mantid::Kernel::LogFilter;
using Mantid::Kernel::Property;
using Mantid::Kernel::TimeSeriesProperty;

namespace {
/// static Logger definition
Mantid::Kernel::Logger g_log("LogFilterGenerator");
} // namespace

namespace Mantid {
namespace API {

/**
 * Constructor
 * @param filterType :: [input] Filter by status, period, both or neither
 * @param workspace :: [input] Workspace containing log data
 */
LogFilterGenerator::LogFilterGenerator(
    const LogFilterGenerator::FilterType filterType,
    const Mantid::API::MatrixWorkspace_const_sptr &workspace)
    : m_filterType(filterType), m_run(workspace->run()) {}

/**
 * Constructor
 * @param filterType :: [input] Filter by status, period, both or neither
 * @param run :: [input] Run containing log data
 */
LogFilterGenerator::LogFilterGenerator(
    const LogFilterGenerator::FilterType filterType,
    const Mantid::API::Run &run)
    : m_filterType(filterType), m_run(run) {}

/**
 * Generate log filter from given workspace and log name
 * @param logName :: [input] Name of log to generate filter for
 * @returns :: LogFilter with selected options
 */
std::unique_ptr<LogFilter>
LogFilterGenerator::generateFilter(const std::string &logName) const {
  const auto *logData = getLogData(logName);
  if (!logData) {
    throw std::invalid_argument("Workspace does not contain log " + logName);
  }

  // This will throw if the log is not a numeric time series.
  // This behaviour is what we want, so don't catch the exception
  auto flt = std::make_unique<LogFilter>(logData);

  switch (m_filterType) {
  case FilterType::None:
    break; // Do nothing
  case FilterType::Period:
    filterByPeriod(flt.get());
    break;
  case FilterType::Status:
    filterByStatus(flt.get());
    break;
  case FilterType::StatusAndPeriod:
    filterByPeriod(flt.get());
    filterByStatus(flt.get());
    break;
  default:
    break;
  }

  return flt;
}

/**
 * Adds a filter to the given LogFilter based on the "running" status log of the
 * workspace.
 * If that log is not present, does nothing.
 * If the filter records start later than the data, add a value of "not running"
 * at the start.
 * @param filter :: [input, output] LogFilter to which filter will be added
 */
void LogFilterGenerator::filterByStatus(LogFilter *filter) const {
  const auto status = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(
      getLogData("running", false));
  if (!status) {
    return;
  }
  filter->addFilter(*status);
  const auto &time_value_map = filter->data()->valueAsCorrectMap();
  const auto &firstTime = time_value_map.begin()->first;
  // If filter records start later than the data we add a value at the
  // filter's front
  if (status->firstTime() > firstTime) {
    // add a "not running" value to the status filter
    Mantid::Kernel::TimeSeriesProperty<bool> atStart("tmp");
    atStart.addValue(firstTime, false);
    atStart.addValue(status->firstTime(), status->firstValue());
    filter->addFilter(atStart);
  }
}

/**
 * Adds filters to the given LogFilter based on the first "period *" log in the
 * workspace.
 * If no such log is present, does nothing.
 * @param filter :: [input, output] LogFilter to which filter will be added
 */
void LogFilterGenerator::filterByPeriod(LogFilter *filter) const {
  const auto &logs = m_run.getLogData();
  for (const auto &log : logs) {
    if (log->name().find("period ") == 0) {
      try {
        const auto periodLog =
            dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(log);
        if (periodLog) {
          filter->addFilter(*periodLog);
        } else {
          g_log.warning("Could not filter by period");
          return;
        }
      } catch (const std::runtime_error &err) {
        g_log.warning() << "Could not filter by period: " << err.what();
        return;
      }
      break;
    }
  }
}

/**
 * Get log data from workspace
 * @param logName :: [input] Name of log to get
 * @param warnIfNotFound :: If true log a warning if the sample log is not
 * found, otherwise do not log a warning
 * @returns :: Pointer to log, or null if log does not exist in workspace
 */
Property *LogFilterGenerator::getLogData(const std::string &logName,
                                         bool warnIfNotFound) const {
  try {
    const auto logData = m_run.getLogData(logName);
    return logData;
  } catch (const std::runtime_error &) {
    if (warnIfNotFound)
      g_log.warning("Could not find log value " + logName + " in workspace");
    return nullptr;
  }
}

} // namespace API
} // namespace Mantid

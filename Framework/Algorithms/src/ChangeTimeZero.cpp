// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/ChangePulsetime.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/lexical_cast.hpp>
#include <memory>
#include <utility>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangeTimeZero)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;
using Types::Core::DateAndTime;

namespace {
/**
 * General check if we are dealing with a time series
 * @param prop :: the property which is being checked
 * @return True if the proerpty is a time series, otherwise false.
 */
bool isTimeSeries(Mantid::Kernel::Property *prop) {
  auto isTimeSeries = false;
  if (dynamic_cast<Mantid::Kernel::ITimeSeriesProperty *>(prop)) {
    isTimeSeries = true;
  }
  return isTimeSeries;
}
} // namespace

/** Initialize the algorithm's properties.
 */
void ChangeTimeZero::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty<double>("RelativeTimeOffset", m_defaultTimeShift, "A relative time offset in seconds.");

  declareProperty("AbsoluteTimeOffset", m_defaultAbsoluteTimeShift,
                  "An absolute time offset as an ISO8601 string "
                  "(YYYY-MM-DDTHH:MM::SS, eg 2013-10-25T13:58:03).");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/** Execute the algorithm.
 */
void ChangeTimeZero::exec() {
  MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");

  // Create a new target workspace if it does not exist
  const double progressStartCreateOutputWs = 0.0;
  const double progressStopCreateOutputWs = 0.3;

  MatrixWorkspace_sptr out_ws = createOutputWS(in_ws, progressStartCreateOutputWs, progressStopCreateOutputWs);

  // Get the time shift in seconds
  auto timeShift = getTimeShift(out_ws);

  // Set up remaining progress points
  const double progressStartShiftTimeLogs = progressStopCreateOutputWs;

  double progressStopShiftTimeLogs =
      std::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(out_ws) ? progressStartShiftTimeLogs + 0.1 : 1.0;

  const double progressStartShiftNeutrons = progressStopShiftTimeLogs;
  const double progressStopShiftNeutrons = 1.0;

  // Change the time of the logs.
  // Initialize progress reporting.
  shiftTimeOfLogs(out_ws, timeShift, progressStartShiftTimeLogs, progressStopShiftTimeLogs);

  // Change the time stamps on the neutrons
  shiftTimeOfNeutrons(out_ws, timeShift, progressStartShiftNeutrons, progressStopShiftNeutrons);

  setProperty("OutputWorkspace", out_ws);
}

/**
 * Create a new output workspace if required
 * @param input :: pointer to an input workspace
 * @param startProgress :: start point of the progress
 * @param stopProgress :: end point of the progress
 * @returns :: pointer to the outputworkspace
 */
API::MatrixWorkspace_sptr ChangeTimeZero::createOutputWS(const API::MatrixWorkspace_sptr &input, double startProgress,
                                                         double stopProgress) {
  MatrixWorkspace_sptr output = getProperty("OutputWorkspace");
  // Check whether input == output to see whether a new workspace is required.
  if (input != output) {
    auto duplicate = createChildAlgorithm("CloneWorkspace", startProgress, stopProgress);
    duplicate->initialize();
    duplicate->setProperty<API::Workspace_sptr>("InputWorkspace", std::dynamic_pointer_cast<API::Workspace>(input));
    duplicate->execute();
    API::Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
    output = std::dynamic_pointer_cast<API::MatrixWorkspace>(temp);
  }
  return output;
}

/**
 * Get the time shift that was specified by the user. If the time is
 * absolute, we need to convert it to relative time.
 * @param ws :: a workspace with time stamp information
 * @returns A time shift in seconds
 */
double ChangeTimeZero::getTimeShift(const API::MatrixWorkspace_sptr &ws) const {
  double timeShift;
  // Check if we are dealing with an absolute time
  std::string timeOffset = getProperty("AbsoluteTimeOffset");
  if (isAbsoluteTimeShift(timeOffset)) {
    DateAndTime desiredTime(timeOffset);
    DateAndTime originalTime(getStartTimeFromWorkspace(ws));
    timeShift = DateAndTime::secondsFromDuration(desiredTime - originalTime);
  } else {
    timeShift = getProperty("RelativeTimeOffset");
  }
  return timeShift;
}

/**
 * Change the time of the logs.
 * @param ws :: a workspace
 * @param timeShift :: the time shift that is applied to the log files
 * @param startProgress :: start point of the progress
 * @param stopProgress :: end point of the progress
 */
void ChangeTimeZero::shiftTimeOfLogs(const Mantid::API::MatrixWorkspace_sptr &ws, double timeShift,
                                     double startProgress, double stopProgress) {
  // We need to change the entries for each log which can be:
  // 1. any time series: here we change the time values
  // 2. string properties: here we change the values if they are ISO8601 times
  auto logs = ws->mutableRun().getLogData();
  Progress prog(this, startProgress, stopProgress, logs.size());
  for (auto &log : logs) {
    if (isTimeSeries(log)) {
      shiftTimeInLogForTimeSeries(ws, log, timeShift);

    } else if (auto stringProperty = dynamic_cast<PropertyWithValue<std::string> *>(log)) {
      shiftTimeOfLogForStringProperty(stringProperty, timeShift);
    }

    prog.report(name());
  }
}

/**
 * Shift the time in a time series. This is similar to the implementation in
 * @param ws :: a matrix workspace
 * @param prop :: a time series log
 * @param timeShift :: the time shift in seconds
 */
void ChangeTimeZero::shiftTimeInLogForTimeSeries(const Mantid::API::MatrixWorkspace_sptr &ws,
                                                 Mantid::Kernel::Property *prop, double timeShift) const {
  if (auto timeSeries = dynamic_cast<Mantid::Kernel::ITimeSeriesProperty *>(prop)) {
    auto newlog = timeSeries->cloneWithTimeShift(timeShift);
    ws->mutableRun().addProperty(newlog, true);
  }
}

/**
 * Shift the times in a log of a string property
 * @param logEntry :: the string property
 * @param timeShift :: the time shift.
 */
void ChangeTimeZero::shiftTimeOfLogForStringProperty(PropertyWithValue<std::string> *logEntry, double timeShift) const {
  // Parse the log entry and replace all ISO8601 strings with an adjusted value
  auto value = logEntry->value();
  if (checkForDateTime(value)) {
    DateAndTime dateTime(value);
    DateAndTime shiftedTime = dateTime + timeShift;
    logEntry->setValue(shiftedTime.toISO8601String());
  }
}

/**
 * Shift the time of the neutrons
 * @param ws :: a matrix workspace
 * @param timeShift :: the time shift in seconds
 * @param startProgress :: start point of the progress
 * @param stopProgress :: end point of the progress
 */
void ChangeTimeZero::shiftTimeOfNeutrons(const Mantid::API::MatrixWorkspace_sptr &ws, double timeShift,
                                         double startProgress, double stopProgress) {
  if (auto eventWs = std::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(ws)) {
    // Use the change pulse time algorithm to change the neutron time stamp
    auto alg = createChildAlgorithm("ChangePulsetime", startProgress, stopProgress);
    alg->initialize();
    alg->setProperty("InputWorkspace", eventWs);
    alg->setProperty("OutputWorkspace", eventWs);
    alg->setProperty("TimeOffset", timeShift);
    alg->execute();
  }
}

/**
 * Extract the first good frame of a workspace
 * @param ws :: a workspace
 * @returns the date and time of the first good frame
 */
DateAndTime ChangeTimeZero::getStartTimeFromWorkspace(const API::MatrixWorkspace_sptr &ws) const {
  auto run = ws->run();
  // Check for the first good frame in the log
  Mantid::Kernel::TimeSeriesProperty<double> *goodFrame = nullptr;
  try {
    goodFrame = run.getTimeSeriesProperty<double>("proton_charge");
  } catch (const std::invalid_argument &) {
    throw std::invalid_argument("ChangeTimeZero: The log needs a proton_charge "
                                "time series to determine the zero time.");
  }

  DateAndTime startTime;
  if (goodFrame->size() > 0) {
    startTime = goodFrame->firstTime();
  }

  return startTime;
}

/**
 * Check the inputs for invalid values
 * @returns A map with validation warnings.
 */
std::map<std::string, std::string> ChangeTimeZero::validateInputs() {
  std::map<std::string, std::string> invalidProperties;

  // Check the time offset for either a value or a date time
  double relativeTimeOffset = getProperty("RelativeTimeOffset");
  std::string absoluteTimeOffset = getProperty("AbsoluteTimeOffset");

  auto isRelative = isRelativeTimeShift(relativeTimeOffset);
  auto absoluteTimeInput = absoluteTimeOffset != m_defaultAbsoluteTimeShift;
  auto isAbsolute = isAbsoluteTimeShift(absoluteTimeOffset);

  // If both inputs are being used, then return straight away.
  if (isRelative && absoluteTimeInput) {
    invalidProperties.emplace("RelativeTimeOffset", "You can either specify a relative time shift or "
                                                    "an absolute time shift.");
    invalidProperties.emplace("AbsoluteTimeOffset", "You can either specify a relative time shift or "
                                                    "an absolute time shift.");

    return invalidProperties;
  } else if (!isRelative && !isAbsolute) {
    invalidProperties.emplace("RelativeTimeOffset", "TimeOffset must either be a numeric "
                                                    "value or a ISO8601 (YYYY-MM-DDTHH:MM::SS) date-time stamp.");
    invalidProperties.emplace("AbsoluteTimeOffset", "TimeOffset must either be a numeric "
                                                    "value or a ISO8601 (YYYY-MM-DDTHH:MM::SS) date-time stamp.");
  }

  // If we are dealing with an absolute time we need to ensure that the
  // proton_charge entry exists
  if (isAbsolute) {
    MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
    if (ws) {
      auto run = ws->run();
      try {
        run.getTimeSeriesProperty<double>("proton_charge");
      } catch (...) {
        invalidProperties.emplace("InputWorkspace", "A TimeOffset with an absolute time requires the "
                                                    "input workspace to have a proton_charge property in "
                                                    "its log.");
      }
    }
  }

  return invalidProperties;
}

/** Can the string be transformed to double
 * @param val :: value to check
 * @return True if the string can be cast to double and otherwise false.
 */
bool ChangeTimeZero::checkForDouble(const std::string &val) const {
  auto isDouble = false;
  try {
    boost::lexical_cast<double>(val);
    isDouble = true;
  } catch (boost::bad_lexical_cast const &) {
  }
  return isDouble;
}

/** Can the string be transformed to a DateTime object
 * @param val :: value to check
 * @return True if the string can be cast to a DateTime object and otherwise
 * false.
 */
bool ChangeTimeZero::checkForDateTime(const std::string &val) const {
  auto isDateTime = false;
  // Hedge for bad lexical casts in the DateTimeValidator
  try {
    DateTimeValidator validator = DateTimeValidator();
    isDateTime = validator.isValid(val).empty();
  } catch (...) {
    isDateTime = false;
  }
  return isDateTime;
}

/**
 * Checks if a relative offset has been set
 * @param offset :: the offset
 * @returns true if the offset has been set
 */
bool ChangeTimeZero::isRelativeTimeShift(double offset) const { return offset != m_defaultTimeShift; }

/**
 * Checks if an absolute offset has been set
 * @param offset :: the offset
 * @returns true if the offset has been set
 */
bool ChangeTimeZero::isAbsoluteTimeShift(const std::string &offset) const {
  return offset != m_defaultAbsoluteTimeShift && checkForDateTime(offset);
}

} // namespace Mantid::Algorithms

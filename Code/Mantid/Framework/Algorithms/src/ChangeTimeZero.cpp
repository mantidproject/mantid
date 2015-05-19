#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DateTimeValidator.h"

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangeTimeZero)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ChangeTimeZero::ChangeTimeZero()
    : m_isDouble(false), m_isDateAndTime(false),
      m_dateTimeValidator(boost::make_shared<DateTimeValidator>()) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ChangeTimeZero::~ChangeTimeZero() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangeTimeZero::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "An input workspace.");
  declareProperty("TimeOffset", "",
                  "A relative offset in seconds or an absolute time.");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ChangeTimeZero::exec() {

  MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");

  // Create a new target workspace if it does not exist
  MatrixWorkspace_sptr out_ws = createOutputWS(in_ws);

  // Get the time shift in seconds
  auto timeShift = getTimeShift(out_ws);

  // Change the time of the logs.
  shiftTimeOfLogs(out_ws, timeShift);

  // Change the time stamps on the neutrons
  shiftTimeOfNeutrons(out_ws, timeShift);

  setProperty("OutputWorkspace", out_ws);
}

/**
 * Create a new output workspace if required
 * @param :: pointer to an input workspace
 * @returns :: pointer to the outputworkspace
 */
API::MatrixWorkspace_sptr
ChangeTimeZero::createOutputWS(API::MatrixWorkspace_sptr input) {
  MatrixWorkspace_sptr output = getProperty("OutputWorkspace");
  // Check whether input == output to see whether a new workspace is required.
  if (input != output) {
    IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace");
    duplicate->initialize();
    duplicate->setProperty<API::Workspace_sptr>(
        "InputWorkspace", boost::dynamic_pointer_cast<API::Workspace>(input));
    duplicate->execute();
    API::Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
    output = boost::dynamic_pointer_cast<API::MatrixWorkspace>(temp);
  }
  return output;
}

/**
 * Get the time shift that was specified by the user. If the time is
 * absolute, we need to convert it to relative time.
 * @param ws :: a workspace with time stamp information
 * @returns A time shift in seconds
 */
double ChangeTimeZero::getTimeShift(API::MatrixWorkspace_sptr ws) const {
  auto timeShift = 0.0;
  std::string timeOffset = getProperty("TimeOffset");

  // Check if we are dealing with an absolute time
  if (m_isDateAndTime) {
    DateAndTime desiredTime(timeOffset);
    DateAndTime originalTime(getStartTimeFromWorkspace(ws));
    time_duration duration = desiredTime - originalTime;
    timeShift = static_cast<double>(duration.seconds());
  } else {
    timeShift = boost::lexical_cast<double>(timeOffset);
  }
  return timeShift;
}

/**
 * Change the time of the logs.
 * @param ws :: a workspace
 * @param timeShift :: the time shift that is applied to the log files
 */
void ChangeTimeZero::shiftTimeOfLogs(Mantid::API::MatrixWorkspace_sptr ws,
                                     double timeShift) {
  // We need to change the entries for each log which can be:
  // 1. any time series: here we change the time values
  // 2. string properties: here we change the values if they are ISO8601 times
  auto logs = ws->run().getLogData();

  for (auto iter = logs.begin(); iter != logs.end(); ++iter) {
    auto prop = ws->run().getLogData((*iter)->name());

    if (isTimeSeries(prop)) {
      shiftTimeInLogForTimeSeries(ws, prop, timeShift);

    } else if (auto stringProperty =
                   dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
      shiftTimeOfLogForStringProperty(ws, stringProperty, timeShift);
    }
  }
}

/**
 * Shift the time in a time series. This is similar to the implementation in
 * @param ws :: a matrix workspace
 * @param prop :: a time series log
 * @param timeShift :: the time shift in seconds
 */
void ChangeTimeZero::shiftTimeInLogForTimeSeries(Mantid::API::MatrixWorkspace_sptr ws,
                                 Mantid::Kernel::Property *prop,
                                 double timeShift) {
  auto timeSeries = dynamic_cast<Mantid::Kernel::ITimeSeriesProperty*>(prop);
  if (!timeSeries) {
    return;
  }
  auto newlog = timeSeries->cloneWithTimeShift(timeShift);
  ws->mutableRun().addProperty(newlog, true);
}

/**
 * Shift the times in a log of a string property
 * @param ws :: a matrix workspace
 * @param logEntry :: the string property
 * @param timeShift :: the time shift.
 */
void ChangeTimeZero::shiftTimeOfLogForStringProperty(
    MatrixWorkspace_sptr ws, PropertyWithValue<std::string> *logEntry,
    double timeShift) {
  // Parse the log entry and replace all ISO8601 strings with an adjusted value
  auto value = logEntry->value();

  auto isDateTime = checkForDateTime(value);
  if (isDateTime) {
    DateAndTime dateTime(value);
    DateAndTime shiftedTime = dateTime + timeShift;
    logEntry->setValue(shiftedTime.toISO8601String());
  } else {
    // TODO: Write warning
  }
}

/**
 * Shift the time of the neutrons
 * @param ws :: a matrix workspace
 * @param timeShift :: the time shift
 */
void ChangeTimeZero::shiftTimeOfNeutrons(Mantid::API::MatrixWorkspace_sptr ws,
                                         double timeShift) {
  // If the matrix workspace is an event workspace we need to change the events
  auto eventWs =
      boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(ws);

  if (!eventWs) {
    return;
  }

  const auto scaleFactorTof = 1.0;

  for (size_t workspaceIndex = 0;
       workspaceIndex < eventWs->getNumberHistograms(); ++workspaceIndex) {
    Mantid::DataObjects::EventList *eventList =
        eventWs->getEventListPtr(workspaceIndex);
    eventList->convertTof(scaleFactorTof, timeShift);
  }
}

/**
 * Release the flag values for double input and date time input
 */
void ChangeTimeZero::resetFlags() {
  m_isDouble = false;
  m_isDateAndTime = false;
}

/**
 * Extract the first good frame of a workspace
 * @param ws :: a workspace
 * @retruns The date and time of the first good frame
 */
DateAndTime
ChangeTimeZero::getStartTimeFromWorkspace(API::MatrixWorkspace_sptr ws) const {
  auto run = ws->run();
  // Check for the first good frame in the log
  auto goodFrame = run.getTimeSeriesProperty<double>("proton_charge");
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

  // Reset flag values
  resetFlags();

  // Check the time offset for either a value or a date time
  std::string timeOffset = getProperty("TimeOffset");

  m_isDouble = checkForDouble(timeOffset);
  m_isDateAndTime = checkForDateTime(timeOffset);

  if (!m_isDouble && !m_isDateAndTime) {
    invalidProperties.insert(
        std::make_pair("TimeOffset", "TimeOffset must either be a numeric "
                                     "value or a IS8601 date-time stamp."));
  }

  return invalidProperties;
}

/** Can the string be transformed to double
 * @param val :: value to check
 * @return True if the string can be cast to double and otherwise false.
 */
bool ChangeTimeZero::checkForDouble(std::string val) {
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
bool ChangeTimeZero::checkForDateTime(std::string val) {
  auto isDateTime = false;
  // Hedge for bad lexical casts in the DateTimeValidator
  try {
    isDateTime = m_dateTimeValidator->isValid(val) == "";
  } catch (...) {
    isDateTime = false;
  }
  return isDateTime;
}

} // namespace Mantid
} // namespace Algorithms

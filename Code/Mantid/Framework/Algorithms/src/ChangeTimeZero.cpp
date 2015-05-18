#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAlgorithms/ChangeLogTime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/BoundedValidator.h"
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


const std::string ChangeTimeZero::timeSeriesID =
     "std::vector<class Mantid::Kernel::TimeValueUnit<";

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

  MatrixWorkspace_const_sptr in_ws = getProperty("InputWorkspace");

  // Create a new target workspace if it does not exist
  MatrixWorkspace_sptr out_ws = createOutputWS(in_ws);

  // Get the time shift in seconds
  double timeShift = getTimeShift(out_ws);

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
ChangeTimeZero::createOutputWS(API::MatrixWorkspace_const_sptr input) {
  MatrixWorkspace_sptr output = getProperty("OutputWorkspace");
  // Check whether input = output to see whether a new workspace is required.
  if (input != output) {
    // Create new workspace for output from old
    output = API::WorkspaceFactory::Instance().create(input);
  }
  return output;
}

/**
 * Get the time shift that was specified by the user. If the the time is
 * absolute, we need to convert it to relative time.
 * @param ws :: a workspace with time stamp information
 * @returns A time shift in seconds
 */
double ChangeTimeZero::getTimeShift(API::MatrixWorkspace_const_sptr ws) const {
  double timeShift = 0.0;
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
 * Change the time of the logs
 * @param ws :: a workspace
 * @param timeShift :: the time shift that is applied to the log files
 */
void ChangeTimeZero::shiftTimeOfLogs(Mantid::API::MatrixWorkspace_sptr ws,
                                     double timeShift) {
  // We need to change the entries for each log which can be:
  // 1. any time series: here we change the time values
  // 2. string properties: here we change the values
  auto logs = ws->run().getLogData();

  for (auto iter = logs.begin(); iter != logs.end(); ++iter) {
    auto prop = ws->run().getLogData((*iter)->name());

    if (isTimeSeries<double>(prop)) {
      shiftTimeInLogForTimeSeries<double>(ws, prop, timeShift);

    } else if (isTimeSeries<std::string>(prop)) {
      shiftTimeInLogForTimeSeries<std::string>(ws, prop, timeShift);

    } else if (auto stringProperty =
                   dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
      shiftTimeOfLogForStringProperty(ws, stringProperty, timeShift);

    } else if (isTimeSeries<int>(prop)) {
      shiftTimeInLogForTimeSeries<int>(ws, prop, timeShift);

    } else if (isTimeSeries<bool>(prop)) {
      shiftTimeInLogForTimeSeries<bool>(ws, prop, timeShift);

    } else {
      // Other logs are not considered, but make sure that we did not miss a 
      // time series
      if (prop->type().find(timeSeriesID)) {
        // TODO: Write warning
      }
    }
  }
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
  std::string value = logEntry->value();

  bool isDateTime = checkForDateTime(value);
  if (isDateTime) {
    DateAndTime dateTime(logEntry->value());
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
  std::cout << ws->getFirstPulseTime() <<std::endl;
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
DateAndTime ChangeTimeZero::getStartTimeFromWorkspace(
    API::MatrixWorkspace_const_sptr ws) const {
  DateAndTime startTime;
  Mantid::API::Run run = ws->run();

  // Check for the first good frame in the log
  TimeSeriesProperty<double> *goodFrame =
      run.getTimeSeriesProperty<double>("proton_charge");
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
  bool isDouble = false;
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
  bool isDateTime = false;
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

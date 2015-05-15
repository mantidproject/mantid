#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAlgorithms/ChangeLogTime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/BoundedValidator.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

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
ChangeTimeZero::ChangeTimeZero() {}

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
  if (DateAndTime::stringIsISO8601(timeOffset)) {
    DateAndTime desiredTime(timeOffset);
    DateAndTime originalTime(getStartTimeFromWorkspace(ws));
    time_duration duration = desiredTime - originalTime;
    timeShift = static_cast<double>(duration.seconds());
  } else {
    try {
      timeShift = boost::lexical_cast<double>(timeOffset);
    } catch (boost::bad_lexical_cast const &) {
      timeShift = 0;
      // TODO: How to handle correctly?
    }
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
  Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ChangeTimeZero");
  ChildAlg->initialize();
  ChildAlg->setProperty("InputWorkspace", ws);
  ChildAlg->setProperty("OutputWorkspace", ws);
  ChildAlg->setProperty("TimeOffset", timeShift);
  ChildAlg->execute();
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
    // TODO: How to handle if there is no first time?
  if (goodFrame->size() > 0) {
    startTime = goodFrame->firstTime();
  }

  return startTime;
}

/**
 * Check the inputs for invalid values
 */
std::map<std::string, std::string> ChangeTimeZero::validateInputs() {
  std::map<std::string, std::string> invalidProperties;

   auto doubleValidator = boost::make_shared<BoundedValidator<double>>();
  // Check the time offset for either a value or a date time
  std::string timeOffset = getProperty("TimeOffset");
  bool isDouble = doubleValidator->isValid(timeOffset) != "";
  bool isDateAndTime = DateAndTime::stringIsISO8601(timeOffset);

  std::cout << "Is double: " <<isDouble <<std::endl;
  std::cout << "Is date and time: " <<isDateAndTime <<std::endl;

  if (!isDouble && !isDateAndTime) {
    invalidProperties.insert(std::make_pair(
        "TimeOffset",
        "TimeOffset must either be a numeric value or a date-time stamp"));
  }

  return invalidProperties;
}

} // namespace Mantid
} // namespace Algorithms

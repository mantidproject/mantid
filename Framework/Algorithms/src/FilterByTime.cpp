// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FilterByTime.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByTime)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;
using Types::Core::DateAndTime;

namespace {
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string START_TIME("StartTime");
const std::string STOP_TIME("StopTime");
const std::string ABS_START("AbsoluteStartTime");
const std::string ABS_STOP("AbsoluteStopTime");
} // namespace PropertyNames
} // namespace

void FilterByTime::init() {
  std::string commonHelp("\nYou can only specify the relative or absolute "
                         "start/stop times, not both.");

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "An input event workspace");

  declareProperty(
      std::make_unique<WorkspaceProperty<EventWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "The name to use for the output workspace");

  auto min = std::make_shared<BoundedValidator<double>>();
  min->setLower(0.0);
  declareProperty(PropertyNames::START_TIME, 0.0, min,
                  "The start time, in seconds, since the start of the run. "
                  "Events before this time are filtered out. \nThe time of the "
                  "first pulse (i.e. the first entry in the ProtonCharge "
                  "sample log) is used as the zero. " +
                      commonHelp);

  declareProperty(PropertyNames::STOP_TIME, 0.0, min,
                  "The stop time, in seconds, since the start of the run. "
                  "Events at or after this time are filtered out. \nThe time "
                  "of the first pulse (i.e. the first entry in the "
                  "ProtonCharge sample log) is used as the zero. " +
                      commonHelp);

  auto dateTime = std::make_shared<DateTimeValidator>();
  dateTime->allowEmpty(true);
  std::string absoluteHelp("Specify date and UTC time in ISO8601 format, e.g. 2010-09-14T04:20:12." + commonHelp);
  declareProperty(PropertyNames::ABS_START, "", dateTime,
                  "Absolute start time; events before this time are filtered out. " + absoluteHelp);

  declareProperty(PropertyNames::ABS_STOP, "", dateTime,
                  "Absolute stop time; events at or after this time are filtered out. " + absoluteHelp);
}

std::map<std::string, std::string> FilterByTime::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::string msg_double_spec("You need to specify either the relative or absolute parameter, but not both");

  if ((!isDefault(PropertyNames::START_TIME)) && (!isDefault(PropertyNames::ABS_START))) {
    errors[PropertyNames::START_TIME] = msg_double_spec;
    errors[PropertyNames::ABS_START] = msg_double_spec;
  }

  if ((!isDefault(PropertyNames::STOP_TIME)) && (!isDefault(PropertyNames::ABS_STOP))) {
    errors[PropertyNames::STOP_TIME] = msg_double_spec;
    errors[PropertyNames::ABS_STOP] = msg_double_spec;
  }

  return errors;
}

/** Executes the algorithm
 */
void FilterByTime::exec() {
  EventWorkspace_const_sptr inputWS = this->getProperty(PropertyNames::INPUT_WKSP);

  // find the start time
  DateAndTime start;
  if (isDefault(PropertyNames::ABS_START)) {
    // time relative to first pulse - this defaults with start of run
    const auto startOfRun = inputWS->getFirstPulseTime();
    const double startRelative = getProperty(PropertyNames::START_TIME);
    start = startOfRun + startRelative;
  } else {
    // absolute start time is specified
    start = DateAndTime(getPropertyValue(PropertyNames::ABS_START));
  }

  // find the stop time
  DateAndTime stop;
  if (!isDefault(PropertyNames::ABS_STOP)) {
    // absolute stop time is specified
    stop = DateAndTime(getPropertyValue(PropertyNames::ABS_STOP));
  } else if (!isDefault(PropertyNames::STOP_TIME)) {
    // time relative to start time
    const double stopRelative = getProperty(PropertyNames::STOP_TIME);
    stop = start + stopRelative;
  } else {
    this->getLogger().debug("No end filter time specified - assuming last pulse");
    const DateAndTime lastPulse = inputWS->getLastPulseTime();
    stop = lastPulse + 10000.0; // so we get all events - needs to be past last pulse
  }

  // verify that stop is after start
  if (stop <= start) {
    std::stringstream msg;
    msg << "The stop time (" << stop << ") should be larger than the start time (" << start << ")";
    throw std::invalid_argument(msg.str());
  }

  auto outputWS = DataObjects::create<EventWorkspace>(*inputWS);

  size_t numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // Get the output event list (should be empty)
    EventList &output_el = outputWS->getSpectrum(i);
    // and this is the input event list
    const EventList &input_el = inputWS->getSpectrum(i);

    // Perform the filtering
    input_el.filterByPulseTime(start, stop, output_el);

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Now filter out the run, using the DateAndTime type.
  auto timeroi = outputWS->mutableRun().getTimeROI(); // make a copy
  if (timeroi.useAll()) {
    // trim in for where it should be used
    timeroi.addROI(start, stop);
  } else {
    // only use the overlap region
    timeroi.update_intersection(TimeROI(start, stop));
  }

  outputWS->mutableRun().setTimeROI(timeroi);
  outputWS->mutableRun().removeDataOutsideTimeROI();
  setProperty(PropertyNames::OUTPUT_WKSP, std::move(outputWS));
}

} // namespace Mantid::Algorithms

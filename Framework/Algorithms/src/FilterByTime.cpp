// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FilterByTime.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByTime)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;
using Types::Core::DateAndTime;

void FilterByTime::init() {
  std::string commonHelp("\nYou can only specify the relative or absolute "
                         "start/stop times, not both.");

  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  auto min = boost::make_shared<BoundedValidator<double>>();
  min->setLower(0.0);
  declareProperty("StartTime", 0.0, min,
                  "The start time, in seconds, since the start of the run. "
                  "Events before this time are filtered out. \nThe time of the "
                  "first pulse (i.e. the first entry in the ProtonCharge "
                  "sample log) is used as the zero. " +
                      commonHelp);

  declareProperty("StopTime", 0.0,
                  "The stop time, in seconds, since the start of the run. "
                  "Events at or after this time are filtered out. \nThe time "
                  "of the first pulse (i.e. the first entry in the "
                  "ProtonCharge sample log) is used as the zero. " +
                      commonHelp);

  std::string absoluteHelp(
      "Specify date and UTC time in ISO8601 format, e.g. 2010-09-14T04:20:12." +
      commonHelp);
  declareProperty(
      "AbsoluteStartTime", "",
      "Absolute start time; events before this time are filtered out. " +
          absoluteHelp);

  declareProperty(
      "AbsoluteStopTime", "",
      "Absolute stop time; events at of after this time are filtered out. " +
          absoluteHelp);
}

/** Executes the algorithm
 */
void FilterByTime::exec() {
  EventWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");

  // ---- Find the start/end times ----
  DateAndTime start, stop;

  double start_dbl, stop_dbl;
  start_dbl = getProperty("StartTime");
  stop_dbl = getProperty("StopTime");

  std::string start_str, stop_str;
  start_str = getPropertyValue("AbsoluteStartTime");
  stop_str = getPropertyValue("AbsoluteStopTime");

  if ((start_str != "") && (stop_str != "") && (start_dbl <= 0.0) &&
      (stop_dbl <= 0.0)) {
    // Use the absolute string
    start = DateAndTime(start_str);
    stop = DateAndTime(stop_str);
  } else if ((start_str == "") && (stop_str == "") &&
             ((start_dbl > 0.0) || (stop_dbl > 0.0))) {
    // Use the relative times in seconds.
    DateAndTime first = inputWS->getFirstPulseTime();
    DateAndTime last = inputWS->getLastPulseTime();
    start = first + start_dbl;
    if (stop_dbl > 0.0) {
      stop = first + stop_dbl;
    } else {
      this->getLogger().debug()
          << "No end filter time specified - assuming last pulse\n";
      stop =
          last + 10000.0; // so we get all events - needs to be past last pulse
    }
  } else {
    // Either both or none were specified
    throw std::invalid_argument("You need to specify either the StartTime or "
                                "StopTime parameters; or both the "
                                "AbsoluteStartTime and AbsoluteStopTime "
                                "parameters; but not other combinations.");
  }

  if (stop <= start)
    throw std::invalid_argument(
        "The stop time should be larger than the start time.");

  auto outputWS = DataObjects::create<EventWorkspace>(*inputWS);

  size_t numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // Get the output event list (should be empty)
    EventList &output_el = outputWS->getSpectrum(i);
    // and this is the input event list
    const EventList &input_el = inputWS->getSpectrum(i);

    // Perform the filtering
    input_el.filterByPulseTime(start, stop, output_el);

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Now filter out the run, using the DateAndTime type.
  outputWS->mutableRun().filterByTime(start, stop);
  setProperty("OutputWorkspace", std::move(outputWS));
}

} // namespace Algorithms
} // namespace Mantid

/*WIKI* 




Filters out events using the entries in the Sample Logs. 

Sample logs consist of a series of <Time, Value> pairs. The first step in filtering is to generate a list of start-stop time intervals that will be kept, using those logs.
* Each log value is compared to the min/max value filters to determine whether it is "good" or not.
** For a single log value that satisfies the criteria at time T, all events between T+-Tolerance (LogBoundary=Centre), or T and T+Tolerance (LogBoundary=Left) are kept.
** If there are several consecutive log values matching the filter, events between T1-Tolerance and T2+Tolerance (LogBoundary=Centre), or T1 and T2+Tolerance (LogBoundary=Left) are kept.
* The filter is then applied to all events in all spectra. Any events with pulse times outside of any "good" time ranges are removed.

There is no interpolation of log values between the discrete sample log times at this time.
However, the log value is assumed to be constant at times before its first point and after its last. For example, if the first temperature measurement was at time=10 seconds and a temperature within the acceptable range, then all events between 0 and 10 seconds will be included also. If a log has a single point in time, then that log value is assumed to be constant for all time and if it falls within the range, then all events will be kept.

==== PulseFilter (e.g. for Veto Pulses) ====

If you select PulseFilter, then events will be filtered OUT in notches around each
time in the selected sample log, and the MinValue/MaxValue parameters are ignored.
For example:

* If you have 3 entries at times:
** 10, 20, 30 seconds.
** A TimeTolerance of 1 second.
* Then the events at the following times will be EXCLUDED from the output:
** 9-11; 19-21; 29-30 seconds.

The typical use for this is to filter out "veto" pulses from a SNS event nexus file.
Some of these files have a sample log called "veto_pulse_time" that only contains
times of the pulses to be rejected. For example, this call will filter out veto
pulses:

 FilterByLogValue(InputWorkspace="ws", OutputWorkspace="ws", LogName="veto_pulse_time", PulseFilter="1")

*WIKI*/

#include "MantidAlgorithms/FilterByLogValue.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>
#include "MantidKernel/TimeSplitter.h"

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByLogValue)

/// Sets documentation strings for this algorithm
void FilterByLogValue::initDocs()
{
  this->setWikiSummary("Filter out events from an [[EventWorkspace]] based on a sample log value satisfying filter criteria. ");
  this->setOptionalMessage("Filter out events from an EventWorkspace based on a sample log value satisfying filter criteria.");
}


using namespace Kernel;
using namespace DataObjects;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;


//========================================================================
//========================================================================
/// (Empty) Constructor
FilterByLogValue::FilterByLogValue()
{
}

/// Destructor
FilterByLogValue::~FilterByLogValue()
{
}

//-----------------------------------------------------------------------
void FilterByLogValue::init()
{
  declareProperty(
    new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::InOut),
    "An input event workspace" );

  declareProperty(
    new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );

  declareProperty("LogName", "ProtonCharge, "
      "Name of the sample log to use to filter.\n"
      "For example, the pulse charge is recorded in 'ProtonCharge'.");

  declareProperty("MinimumValue", 0.0, "Minimum log value for which to keep events.");

  declareProperty("MaximumValue", 0.0, "Maximum log value for which to keep events.");

  BoundedValidator<double> *min = new BoundedValidator<double>();
  min->setLower(0.0);
  declareProperty("TimeTolerance", 0.0, min,
    "Tolerance, in seconds, for the event times to keep. A good value is 1/2 your measurement interval. \n"
    "For a single log value at time T, all events between T+-Tolerance are kept.\n"
    "If there are several consecutive log values matching the filter, events between T1-Tolerance and T2+Tolerance are kept.");

  std::vector<std::string> types(2);
  types.push_back("Centre");
  types.push_back("Left");
  declareProperty("LogBoundary", "Centre", new Mantid::Kernel::ListValidator(types),
                  "How to treat log values as being measured in the centre of the time, or beginning (left) boundary");


  declareProperty("PulseFilter", false,
    "Optional. Filter out a notch of time for each entry in the sample log named.\n"
    "A notch of width 2*TimeTolerance is centered at each log time. The value of the log is NOT used."
    "This is used, for example, to filter out veto pulses.");


}


//-----------------------------------------------------------------------
/** Executes the algorithm
 */
void FilterByLogValue::exec()
{

  // convert the input workspace into the event workspace we already know it is
  EventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // Get the properties.
  double min = getProperty("MinimumValue");
  double max = getProperty("MaximumValue");
  double tolerance = getProperty("TimeTolerance");
  std::string logname = getPropertyValue("LogName");
  bool PulseFilter = getProperty("PulseFilter");

  // Find the start and stop times of the run, but handle it if they are not found.
  DateAndTime run_start(0), run_stop("2100-01-01");
  double handle_edge_values = false;
  try
  {
    run_start = inputWS->getFirstPulseTime() - tolerance;
    run_stop = inputWS->getLastPulseTime() + tolerance;
    handle_edge_values = true;
  }
  catch (Exception::NotFoundError & )
  {
  }

  // Now make the splitter vector
  TimeSplitterType splitter;
  //This'll throw an exception if the log doesn't exist. That is good.
  Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( inputWS->run().getLogData(logname) );
  if (log)
  {
    if (PulseFilter)
    {
      // ----- Filter at pulse times only -----
      DateAndTime lastTime = run_start;
      std::vector<DateAndTime> times = log->timesAsVector();
      std::vector<DateAndTime>::iterator it;
      for (it = times.begin(); it != times.end(); it++)
      {
        SplittingInterval interval(lastTime, *it - tolerance, 0);
        // Leave a gap +- tolerance
        lastTime = (*it + tolerance);
        splitter.push_back(interval);
      }
      // And the last one
      splitter.push_back(SplittingInterval(lastTime, run_stop, 0));

    }
    else
    {
      // ----- Filter by value ------
      if (max <= min)
        throw std::invalid_argument("MaximumValue should be > MinimumValue. Aborting.");

      //This function creates the splitter vector we will use to filter out stuff.
      std::string logBoundary(this->getPropertyValue("LogBoundary"));
      std::transform(logBoundary.begin(), logBoundary.end(), logBoundary.begin(), tolower);
      log->makeFilterByValue(splitter, min, max, tolerance, (logBoundary.compare("centre") == 0));

      if (log->realSize() >= 1 && handle_edge_values)
      {
        double val;
        // Assume everything before the 1st value is constant
        val = log->firstValue();
        if ((val >= min) && (val <= max))
        {
          TimeSplitterType extraFilter;
          extraFilter.push_back( SplittingInterval(run_start, log->firstTime(), 0));
          // Include everything from the start of the run to the first time measured (which may be a null time interval; this'll be ignored)
          splitter = splitter | extraFilter;
        }

        // Assume everything after the LAST value is constant
        val = log->lastValue();
        if ((val >= min) && (val <= max))
        {
          TimeSplitterType extraFilter;
          extraFilter.push_back( SplittingInterval(log->lastTime(), run_stop, 0) );
          // Include everything from the start of the run to the first time measured (which may be a null time interval; this'll be ignored)
          splitter = splitter | extraFilter;
        }
      }
    } // (filter by value)



  }

  g_log.information() << splitter.size() << " entries in the filter.\n";
  size_t numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this,0.0,1.0,numberOfSpectra);



  EventWorkspace_sptr outputWS;
  if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"))
  {
    // Filtering in place! -------------------------------------------------------------
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i)
    {
      PARALLEL_START_INTERUPT_REGION

      // this is the input event list
      EventList& input_el = inputWS->getEventList(i);

      // Perform the filtering in place.
      input_el.filterInPlace(splitter);

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    //To split/filter the runs, first you make a vector with just the one output run
    std::vector< Run *> output_runs;
    Run * output_run = new Run(inputWS->mutableRun());
    output_runs.push_back( output_run );
    inputWS->run().splitByTime(splitter, output_runs);
    // Set the output back in the input
    inputWS->mutableRun() = *output_runs[0];
    inputWS->mutableRun().integrateProtonCharge();

    //Cast the outputWS to the matrixOutputWS and save it
    this->setProperty("OutputWorkspace", inputWS);
  }
  else
  {
    //Make a brand new EventWorkspace for the output ------------------------------------------------------
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //But we don't copy the data.

    // Loop over the histograms (detector spectra)
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i)
    {
      PARALLEL_START_INTERUPT_REGION

      //Get the output event list (should be empty)
      EventList * output_el = outputWS->getEventListPtr(i);
      std::vector< EventList * > outputs;
      outputs.push_back(output_el);

      //and this is the input event list
      const EventList& input_el = inputWS->getEventList(i);

      //Perform the filtering (using the splitting function and just one output)
      input_el.splitByTime(splitter, outputs);

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    outputWS->doneAddingEventLists();

    //To split/filter the runs, first you make a vector with just the one output run
    std::vector< Run *> output_runs;
    output_runs.push_back( &outputWS->mutableRun() );
    inputWS->run().splitByTime(splitter, output_runs);

    //Cast the outputWS to the matrixOutputWS and save it
    this->setProperty("OutputWorkspace", outputWS);
  }



}


} // namespace Algorithms
} // namespace Mantid

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByLogValue)

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
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  //Workspace must be an Event workspace
  wsValidator->add(new API::EventWorkspaceValidator<MatrixWorkspace>);

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "An input event workspace" );

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
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

}


//-----------------------------------------------------------------------
/** Executes the algorithm
 */
void FilterByLogValue::exec()
{

  // convert the input workspace into the event workspace we already know it is
  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS
                 = boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);
  if (!inputWS)
  {
    throw std::invalid_argument("Input workspace is not an EventWorkspace. Aborting.");
  }

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //But we don't copy the data.

    //Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Get the properties.
  double min = getProperty("MinimumValue");
  double max = getProperty("MaximumValue");
  double tolerance = getProperty("TimeTolerance");
  std::string logname = getPropertyValue("LogName");

  if (max <= min)
    throw std::invalid_argument("MaximumValue should be > MinimumValue. Aborting.");

  // Now make the splitter vector
  TimeSplitterType splitter;
  //This'll throw an exception if the log doesn't exist. That is good.
  Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( inputWS->run().getLogData(logname) );
  if (log)
  {
    //This function creates the splitter vector we will use to filter out stuff.
    log->makeFilterByValue(splitter, min, max, tolerance);
    // for (int i=0; i < splitter.size(); i++)   std::cout << splitter[i].start() << " to " << splitter[i].stop() << "\n";
  }

  g_log.information() << splitter.size() << " entries in the filter.\n";

  int numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this,0.0,1.0,numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < numberOfSpectra; ++i)
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

}


} // namespace Algorithms
} // namespace Mantid

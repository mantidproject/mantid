//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FilterByTime.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByTime)

/// Sets documentation strings for this algorithm
void FilterByTime::initDocs()
{
  this->setWikiSummary("This algorithm filters out events from an EventWorkspace that are not between given start and stop times. ");
  this->setOptionalMessage("This algorithm filters out events from an EventWorkspace that are not between given start and stop times.");
}


using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;


//========================================================================
//========================================================================
/// (Empty) Constructor
FilterByTime::FilterByTime()
{
}

/// Destructor
FilterByTime::~FilterByTime()
{
}

//-----------------------------------------------------------------------
void FilterByTime::init()
{
  std::string commonHelp("\nYou can only specify the relative or absolute start/stop times, not both.");

  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  //Workspace must be an Event workspace
  wsValidator->add(new API::EventWorkspaceValidator<MatrixWorkspace>);

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "An input event workspace" );

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );


  BoundedValidator<double> *min = new BoundedValidator<double>();
  min->setLower(0.0);
  declareProperty("StartTime", 0.0, min,
      "The start time, in seconds, since the start of the run. Events before this time are filtered out. \nThe time of the first pulse (i.e. the first entry in the ProtonCharge sample log) is used as the zero. " + commonHelp);

  declareProperty("StopTime", 0.0,
      "The stop time, in seconds, since the start of the run. Events at or after this time are filtered out. \nThe time of the first pulse (i.e. the first entry in the ProtonCharge sample log) is used as the zero. " + commonHelp);

  std::string absoluteHelp("Specify date and UTC time in ISO8601 format, e.g. 2010-09-14T04:20:12." + commonHelp);
  declareProperty("AbsoluteStartTime", "",
    "Absolute start time; events before this time are filtered out. " + absoluteHelp );

  declareProperty("AbsoluteStopTime", "",
    "Absolute stop time; events at of after this time are filtered out. " + absoluteHelp );

}


//-----------------------------------------------------------------------
/** Executes the algorithm
 */
void FilterByTime::exec()
{

  // convert the input workspace into the event workspace we already know it is
  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS
                 = boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);
  if (!inputWS)
  {
    throw std::invalid_argument("Input workspace is not an EventWorkspace. Aborting.");
  }


  // ---- Find the start/end times ----
  DateAndTime start, stop;

  double start_dbl, stop_dbl;
  start_dbl = getProperty("StartTime");
  stop_dbl = getProperty("StopTime");

  std::string start_str, stop_str;
  start_str = getPropertyValue("AbsoluteStartTime");
  stop_str = getPropertyValue("AbsoluteStopTime");

  if ((start_str!="") && (stop_str!="") && (stop_dbl<=0.0) && (stop_dbl<=0.0))
  {
    //Use the absolute string
    start = DateAndTime( start_str );
    stop  = DateAndTime( stop_str );
  }
  else
  if ((start_str=="") && (stop_str=="") && (stop_dbl> 0.0) && (stop_dbl> 0.0))
  {
    //Use the relative times in seconds.
    DateAndTime first = inputWS->getFirstPulseTime();
    start = first + start_dbl;
    stop = first + stop_dbl;
  }
  else
  {
    //Either both or none were specified
    throw std::invalid_argument("You need to specify either: both the StartTime and StopTime parameters; or both the AbsoluteStartTime and AbsoluteStopTime parameters; but not all four.");
  }

  if (stop <= start)
    throw std::invalid_argument("The stop time should be larger than the start time.");



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



  size_t numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this,0.0,1.0,numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    //Get the output event list (should be empty)
    EventList& output_el = outputWS->getEventList(i);
    //and this is the input event list
    const EventList& input_el = inputWS->getEventList(i);

    //Perform the filtering
    input_el.filterByPulseTime(start, stop, output_el);

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->doneAddingEventLists();

  //Now filter out the run, using the DateAndTime type.
  outputWS->mutableRun().filterByTime(start, stop);

}


} // namespace Algorithms
} // namespace Mantid

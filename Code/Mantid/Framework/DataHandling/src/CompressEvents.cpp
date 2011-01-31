//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/CompressEvents.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <set>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(CompressEvents)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// (Empty) Constructor
CompressEvents::CompressEvents() {}

/// Destructor
CompressEvents::~CompressEvents() {}

void CompressEvents::init()
{
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::InOut,
    new EventWorkspaceValidator<>),
    "The name of the EventWorkspace on which to perform the algorithm");

  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
    "The name of the output EventWorkspace.");

  // Tolerance must be >= 0.0
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
  mustBePositive->setLower(0.0);
  declareProperty(  new PropertyWithValue<double>("Tolerance", 1e-5, mustBePositive, Direction::Input),
    "The tolerance on each event's X value (normally TOF, but may be a different unit if you have used ConvertUnits).\n"
    "Any events within Tolerance will be summed into a single event.");
}


void CompressEvents::exec()
{
  // Get the input workspace
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  double tolerance = getProperty("Tolerance");

  // Some starting things
  bool inplace = (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"));
  int noSpectra = inputWS->getNumberHistograms();
  Progress prog(this,0.0,1.0, noSpectra*2);

  // Sort the input workspace in-place by TOF. This can be faster if there are few event lists.
  inputWS->sortAll(TOF_SORT, &prog);

  // Are we making a copy of the input workspace?
  if (!inplace)
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    // We DONT copy the data though

    // Loop over the histograms (detector spectra)
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < noSpectra; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      // The input event list
      EventList& input_el = inputWS->getEventList(i);
      // And on the output side
      EventList & output_el = outputWS->getOrAddEventList(i);
      // Copy other settings into output
      output_el.setX( input_el.getRefX() );

      // The EventList method does the work.
      input_el.compressEvents(tolerance, &output_el);

      prog.report("Compressing");
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

  }

  else
  {
    // ---- In-place -----
    // Loop over the histograms (detector spectra)
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < noSpectra; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      // The input (also output) event list
      EventList * output_el = outputWS->getEventListPtr(i);
      if (output_el)
      {
        // The EventList method does the work.
        output_el->compressEvents(tolerance, output_el);
      }
      prog.report("Compressing");
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  //Cast to the matrixOutputWS and save it
  this->setProperty("OutputWorkspace", outputWS);



}

} // namespace DataHandling
} // namespace Mantid


/*WIKI*




This algorithm starts by sorting the event lists by TOF; therefore you may gain speed by calling [[Sort]] beforehand. Starting from the smallest TOF, all events within Tolerance are considered to be identical. Pulse times are ignored. A weighted event without time information is created; its TOF is the average value of the summed events; its weight is the sum of the weights of the input events; its error is the sum of the square of the errors of the input events.

Note that using CompressEvents may introduce errors if you use too large of a tolerance. Rebinning an event workspace still uses an all-or-nothing view: if the TOF of the event is in the bin, then the counts of the bin is increased by the event's weight. If your tolerance is large enough that the compound event spans more than one bin, then you will get small differences in the final histogram.

If you are working from the raw events with TOF resolution of 0.100 microseconds, then you can safely use a tolerance of, e.g., 0.05 microseconds to group events together. In this case, histograms with/without compression are identical. If your workspace has undergone changes to its X values (unit conversion for example), you have to use your best judgement for the Tolerance value.






*WIKI*/
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

/// Sets documentation strings for this algorithm
void CompressEvents::initDocs()
{
  this->setWikiSummary("Reduce the number of events in an [[EventWorkspace]] by grouping together events with identical or similar X-values (time-of-flight). ");
  this->setOptionalMessage("Reduce the number of events in an EventWorkspace by grouping together events with identical or similar X-values (time-of-flight).");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// (Empty) Constructor
CompressEvents::CompressEvents() {}

/// Destructor
CompressEvents::~CompressEvents() {}

void CompressEvents::init()
{
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input),
    "The name of the EventWorkspace on which to perform the algorithm");

  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
    "The name of the output EventWorkspace.");

  // Tolerance must be >= 0.0
  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
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
  bool inplace = (inputWS == outputWS);
  const int noSpectra = static_cast<int>(inputWS->getNumberHistograms());
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

    // Do we want to parallelize over event lists, or in each event list
    bool parallel_in_each = noSpectra < PARALLEL_GET_MAX_THREADS;
    //parallel_in_each = false;

    // Loop over the histograms (detector spectra)
    // Don't parallelize the loop if we are going to parallelize each event list.
    // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule(dynamic) if (!parallel_in_each) )
    for (int i = 0; i < noSpectra; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      //the loop variable i can't be signed because of OpenMp rules inforced in Linux. Using this signed type suppresses warnings below
      const size_t index = static_cast<size_t>(i);
      // The input event list
      EventList& input_el = inputWS->getEventList(index);
      // And on the output side
      EventList & output_el = outputWS->getOrAddEventList(index);
      // Copy other settings into output
      output_el.setX( input_el.ptrX() );

      // The EventList method does the work.
      input_el.compressEvents(tolerance, &output_el, parallel_in_each);

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
    for (int64_t i = 0; i < noSpectra; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      // The input (also output) event list
      EventList * output_el =outputWS->getEventListPtr(static_cast<size_t>(i));
      if (output_el)
      {
        // The EventList method does the work.
        output_el->compressEvents(tolerance, output_el);
        Mantid::API::MemoryManager::Instance().releaseFreeMemory();
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


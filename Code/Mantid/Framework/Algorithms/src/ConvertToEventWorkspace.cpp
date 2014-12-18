#include "MantidAlgorithms/ConvertToEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToEventWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertToEventWorkspace::ConvertToEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToEventWorkspace::~ConvertToEventWorkspace() {}

//------------------------------------------MaxEventsPerBin----------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToEventWorkspace::init() {
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace", "",
                                                     Direction::Input),
                  "An input Workspace2D.");
  declareProperty("GenerateZeros", false,
                  "Generate an event even for empty bins\n"
                  "Warning! This may use significantly more memory.");
  declareProperty("GenerateMultipleEvents", false,
                  "Generate a number of evenly spread events in each bin. See "
                  "the help for details.\n"
                  "Warning! This may use significantly more memory.");
  declareProperty(
      "MaxEventsPerBin", 10,
      "If GenerateMultipleEvents is true, specifies a maximum number of events "
      "to generate in a single bin.\n"
      "Use a value that matches your instrument's TOF resolution. Default 10.");
  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Name of the output EventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToEventWorkspace::exec() {
  Workspace2D_const_sptr inWS = getProperty("InputWorkspace");

  bool GenerateMultipleEvents = getProperty("GenerateMultipleEvents");
  bool GenerateZeros = getProperty("GenerateZeros");
  int MaxEventsPerBin = getProperty("MaxEventsPerBin");

  // Create the event workspace
  EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", inWS->getNumberHistograms(), inWS->blocksize() + 1,
          inWS->blocksize()));

  // Copy geometry, etc. over.
  API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

  Progress prog(this, 0.0, 1.0, inWS->getNumberHistograms());
  PARALLEL_FOR1(inWS)
  for (int iwi = 0; iwi < int(inWS->getNumberHistograms()); iwi++) {
    PARALLEL_START_INTERUPT_REGION
    size_t wi = size_t(iwi);

    // The input spectrum (a histogram)
    const ISpectrum *inSpec = inWS->getSpectrum(wi);

    // The output event list
    EventList &el = outWS->getEventList(wi);

    // This method fills in the events
    el.createFromHistogram(inSpec, GenerateZeros, GenerateMultipleEvents,
                           MaxEventsPerBin);

    prog.report("Converting");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Set the output
  setProperty("OutputWorkspace", outWS);
}

} // namespace Mantid
} // namespace Algorithms

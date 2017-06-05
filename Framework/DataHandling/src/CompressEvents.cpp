#include "MantidDataHandling/CompressEvents.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include "tbb/parallel_for.h"

#include <set>
#include <numeric>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(CompressEvents)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void CompressEvents::init() {
  declareProperty(
      make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "",
                                                     Direction::Input),
      "The name of the EventWorkspace on which to perform the algorithm");

  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output EventWorkspace.");

  // Tolerance must be >= 0.0
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(
      make_unique<PropertyWithValue<double>>("Tolerance", 1e-5, mustBePositive,
                                             Direction::Input),
      "The tolerance on each event's X value (normally TOF, but may be a "
      "different unit if you have used ConvertUnits).\n"
      "Any events within Tolerance will be summed into a single event.");
}

void CompressEvents::exec() {
  // Get the input workspace
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  double tolerance = getProperty("Tolerance");

  // Some starting things
  bool inplace = (inputWS == outputWS);
  const size_t noSpectra = inputWS->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, noSpectra * 2);

  // Sort the input workspace in-place by TOF. This can be faster if there are
  // few event lists.
  inputWS->sortAll(TOF_SORT, &prog);

  // Are we making a copy of the input workspace?
  if (!inplace) {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(*inputWS, *outputWS,
                                                           false);
    // We DONT copy the data though
    // Loop over the histograms (detector spectra)
    tbb::parallel_for(tbb::blocked_range<size_t>(0, noSpectra),
                      [tolerance, &inputWS, &outputWS, &prog](
                          const tbb::blocked_range<size_t> &range) {
                        for (size_t index = range.begin(); index < range.end();
                             ++index) {
                          // The input event list
                          EventList &input_el = inputWS->getSpectrum(index);
                          // And on the output side
                          EventList &output_el = outputWS->getSpectrum(index);
                          // Copy other settings into output
                          output_el.setX(input_el.ptrX());
                          // The EventList method does the work.
                          input_el.compressEvents(tolerance, &output_el);
                          prog.report("Compressing");
                        }
                      });
  } else {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, noSpectra),
        [tolerance, &outputWS, &prog](const tbb::blocked_range<size_t> &range) {
          for (size_t index = range.begin(); index < range.end(); ++index) {
            // The input (also output) event list
            auto &output_el = outputWS->getSpectrum(index);
            // The EventList method does the work.
            output_el.compressEvents(tolerance, &output_el);
            prog.report("Compressing");
          }
        });
  }
  // Cast to the matrixOutputWS and save it
  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace DataHandling
} // namespace Mantid

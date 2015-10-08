#include "MantidAlgorithms/FilterByXValue.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FilterByXValue)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Constructor
FilterByXValue::FilterByXValue() {}

/// Destructor
FilterByXValue::~FilterByXValue() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string FilterByXValue::name() const { return "FilterByXValue"; }
/// Algorithm's version for identification. @see Algorithm::version
int FilterByXValue::version() const { return 1; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string FilterByXValue::category() const {
  return "Events\\EventFiltering";
}

void FilterByXValue::init() {
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "The input workspace.");
  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The output workspace.");
  declareProperty("XMin", EMPTY_DBL(), "The minimum X value (in the units of "
                                       "the input workspace) for which events "
                                       "will be retained\n"
                                       "(default: event list min)");
  declareProperty("XMax", EMPTY_DBL(), "The maximum X value (in the units of "
                                       "the input workspace) for which events "
                                       "will be retained. Must be greater than "
                                       "XMin.\n"
                                       "(default: event list max)");
}

std::map<std::string, std::string> FilterByXValue::validateInputs() {
  std::map<std::string, std::string> errors;

  const double xmin = getProperty("XMin");
  const double xmax = getProperty("XMax");

  if (isEmpty(xmin) && isEmpty(xmax)) {
    errors["XMin"] = "At least one of XMin/XMax must be specified.";
    errors["XMax"] = "At least one of XMin/XMax must be specified.";
    return errors;
  }

  if (!isEmpty(xmin) && !isEmpty(xmax) && xmax <= xmin) {
    errors["XMin"] = "XMin must be less than XMax.";
    errors["XMax"] = "XMin must be less than XMax.";
  }

  return errors;
}

void FilterByXValue::exec() {
  // Get the properties
  EventWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  double xmin = getProperty("XMin");
  double xmax = getProperty("XMax");
  // We need to reduce XMin & increase XMax slightly as we want to keep events
  // with exactly those values
  xmin *= 0.999999999;
  xmax *= 1.000000001;

  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  // Check if we're doing thing in-place.
  if (inputWS != outputWS) {
    // Create a new output workspace if not doing things in place. Preserve
    // event-ness.
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        WorkspaceFactory::Instance().create("EventWorkspace", numSpec,
                                            inputWS->blocksize() +
                                                inputWS->isHistogramData(),
                                            inputWS->blocksize()));
    WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    // Copy over the data.
    // TODO: Make this more efficient by only copying over the events that pass
    // the
    // filter rather than copying everything and then removing some. This should
    // entail new methods (e.g. iterators) on EventList as this algorithm
    // shouldn't
    // need to know about the type of the events (e.g. weighted).
    outputWS->copyDataFrom(*inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  Progress prog(this, 0.0, 1.0, numSpec);
  // Loop over the workspace, removing the events that don't pass the filter
  PARALLEL_FOR1(outputWS)
  for (int spec = 0; spec < numSpec; ++spec) {
    PARALLEL_START_INTERUPT_REGION

    EventList &events = outputWS->getEventList(spec);
    // Sort to make getting the tof min/max faster (& since maskTof will sort
    // anyway)
    events.sortTof();
    if (!isEmpty(xmin)) {
      const double list_xmin = events.getTofMin();
      if (xmin > list_xmin)
        events.maskTof(list_xmin, xmin);
      // Despite the name, maskTof really only does filtering which is what we
      // want
    }
    if (!isEmpty(xmax)) {
      const double list_xmax = events.getTofMax();
      // Need to scale up list_xmax slightly to avoid retaining the last event
      // in the list
      if (xmax < list_xmax)
        events.maskTof(xmax, list_xmax * 1.000000001);
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid

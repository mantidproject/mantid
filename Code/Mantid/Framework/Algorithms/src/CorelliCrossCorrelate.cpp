#include "MantidAlgorithms/CorelliCrossCorrelate.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorelliCrossCorrelate)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CorelliCrossCorrelate::CorelliCrossCorrelate() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CorelliCrossCorrelate::~CorelliCrossCorrelate() {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CorelliCrossCorrelate::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<InstrumentValidator>();

  declareProperty(new WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace.");
  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");

  declareProperty("TimingOffset", EMPTY_INT(),
                  boost::make_shared<MandatoryValidator<int>>(),
                  "Correlation chopper TDC timing offset in nanoseconds.");
}

// Validate inputs workspace first.
std::map<std::string, std::string> CorelliCrossCorrelate::validateInputs() {
  std::map<std::string, std::string> errors;

  inputWS = getProperty("InputWorkspace");

  // check for null pointers - this is to protect against workspace groups
  if (!inputWS) {
    return errors;
  }

  // This algorithm will only work for CORELLI, check for CORELLI.
  if (inputWS->getInstrument()->getName() != "CORELLI")
    errors["InputWorkspace"] = "This Algorithm will only work for Corelli.";

  // Must include the correlation-chopper in the IDF.
  else if (!inputWS->getInstrument()->getComponentByName("correlation-chopper"))
    errors["InputWorkspace"] = "Correlation chopper not found.";

  // The chopper must have a sequence parameter
  else if (inputWS->getInstrument()
               ->getComponentByName("correlation-chopper")
               ->getStringParameter("sequence")
               .empty())
    errors["InputWorkspace"] =
        "Found the correlation chopper but no chopper sequence?";

  // Check for the sample and source.
  else if (!inputWS->getInstrument()->getSource() ||
           !inputWS->getInstrument()->getSample())
    errors["InputWorkspace"] = "Instrument not sufficiently defined: failed to "
                               "get source and/or sample";

  // Must include the chopper4 TDCs.
  else if (!inputWS->run().hasProperty("chopper4_TDC"))
    errors["InputWorkspace"] = "Workspace is missing chopper4 TDCs.";

  // Check if input workspace is sorted.
  else if (inputWS->getSortType() == UNSORTED)
    errors["InputWorkspace"] = "The workspace needs to be a sorted.";

  // Check event type for pulse times
  else if (inputWS->getEventType() == WEIGHTED_NOTIME)
    errors["InputWorkspace"] = "This workspace has no pulse time information.";

  return errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CorelliCrossCorrelate::exec() {
  inputWS = getProperty("InputWorkspace");
  outputWS = getProperty("OutputWorkspace");

  if (outputWS != inputWS) {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS,
                                                           false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));
  }

  // Read in chopper sequence from IDF.
  // Chopper sequence, alternating between open and closed. If index%2==0 than
  // absorbing else transparent.
  IComponent_const_sptr chopper =
      inputWS->getInstrument()->getComponentByName("correlation-chopper");
  std::vector<std::string> chopperSequence =
      chopper->getStringParameter("sequence");
  g_log.information("Found chopper sequence: " + chopperSequence[0]);

  std::vector<std::string> chopperSequenceSplit;
  boost::split(chopperSequenceSplit, chopperSequence[0], boost::is_space());

  std::vector<double> sequence;
  sequence.resize(chopperSequenceSplit.size());
  sequence[0] = boost::lexical_cast<double>(chopperSequenceSplit[0]);

  // Need the cumulative sum of the chopper sequence and total transparent
  double totalOpen = 0;
  for (unsigned int i = 1; i < chopperSequenceSplit.size(); i++) {
    double seqAngle = boost::lexical_cast<double>(chopperSequenceSplit[i]);
    sequence[i] = sequence[i - 1] + seqAngle;
    if (i % 2 == 1)
      totalOpen += seqAngle;
  }

  // Calculate the duty cycle and the event weights from the duty cycle.
  double dutyCycle = totalOpen / sequence.back();
  float weightTransparent = static_cast<float>(1.0 / dutyCycle);
  float weightAbsorbing = static_cast<float>(-1.0 / (1.0 - dutyCycle));
  g_log.information() << "dutyCycle = " << dutyCycle
                      << " weightTransparent = " << weightTransparent
                      << " weightAbsorbing = " << weightAbsorbing << "\n";

  // Read in the TDC timings for the correlation chopper and apply the timing
  // offset.
  std::vector<DateAndTime> tdc =
      dynamic_cast<ITimeSeriesProperty *>(
          inputWS->run().getLogData("chopper4_TDC"))->timesAsVector();
  int offset_int = getProperty("TimingOffset");
  const int64_t offset = static_cast<int64_t>(offset_int);
  for (unsigned long i = 0; i < tdc.size(); ++i)
    tdc[i] += offset;

  // Determine period from TDC.
  double period = static_cast<double>(tdc[tdc.size() - 1].totalNanoseconds() -
                                      tdc[1].totalNanoseconds()) /
                  double(tdc.size() - 2);
  g_log.information() << "Frequency = " << 1e9 / period
                      << "Hz Period = " << period << "ns\n";

  // Get the sample and source, calculate distances.
  IComponent_const_sptr sample = inputWS->getInstrument()->getSample();
  const double distanceChopperToSource =
      inputWS->getInstrument()->getSource()->getDistance(*chopper);
  const double distanceSourceToSample =
      inputWS->getInstrument()->getSource()->getDistance(*sample);

  // extract formula from instrument parameters
  std::vector<std::string> t0_formula =
      inputWS->getInstrument()->getStringParameter("t0_formula");
  if (t0_formula.empty())
    throw Exception::InstrumentDefinitionError(
        "Unable to retrieve t0_formula among instrument parameters");
  std::string formula = t0_formula[0];
  g_log.debug() << formula << "\n";

  const double m_convfactor = 0.5e+12 * Mantid::PhysicalConstants::NeutronMass /
                              Mantid::PhysicalConstants::meV;

  // Do the cross correlation.
  int64_t numHistograms = static_cast<int64_t>(inputWS->getNumberHistograms());
  g_log.notice("Start cross-correlation\n");
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION

    EventList *evlist = outputWS->getEventListPtr(i);
    IDetector_const_sptr detector = inputWS->getDetector(i);

    switch (evlist->getEventType()) {
    case TOF:
      // Switch to weights if needed.
      evlist->switchTo(WEIGHTED);
    /* no break */
    // Fall through
    case WEIGHTED:
      break;
    case WEIGHTED_NOTIME:
      // Should never get here
      throw std::runtime_error(
          "This event list has no pulse time information.");
      break;
    }

    std::vector<WeightedEvent> &events = evlist->getWeightedEvents();

    // Skip if empty.
    if (events.empty())
      continue;

    // Check for duplicate pulse problem in Corelli.
    DateAndTime emptyTime;
    if (events.back().pulseTime() == emptyTime)
      throw std::runtime_error(
          "Missing pulse times on events. This will not work.");

    // Scale for elastic scattering.
    double distanceSourceToDetector =
        distanceSourceToSample + detector->getDistance(*sample);
    double tofScale = distanceChopperToSource / distanceSourceToDetector;

    double E1;
    mu::Parser parser;
    parser.DefineVar("incidentEnergy",
                     &E1); // associate variable E1 to this parser
    parser.SetExpr(formula);

    uint64_t tdc_i = 0;
    std::vector<WeightedEvent>::iterator it;
    for (it = events.begin(); it != events.end(); ++it) {
      double tof = it->tof();
      E1 = m_convfactor * (distanceSourceToDetector / tof) *
           (distanceSourceToDetector / tof);
      double t0 = parser.Eval();

      DateAndTime tofTime =
          it->pulseTime() +
          static_cast<int64_t>(((tof - t0) * tofScale + t0) * 1000.);
      while (tdc_i != tdc.size() && tofTime > tdc[tdc_i])
        tdc_i += 1;

      double angle = 360. *
                     static_cast<double>(tofTime.totalNanoseconds() -
                                         tdc[tdc_i - 1].totalNanoseconds()) /
                     period;

      std::vector<double>::iterator location;
      location = std::lower_bound(sequence.begin(), sequence.end(), angle);

      if ((location - sequence.begin()) % 2 == 0) {
        it->m_weight *= weightAbsorbing;
        it->m_errorSquared *= weightAbsorbing * weightAbsorbing;
      } else {
        it->m_weight *= weightTransparent;
        it->m_errorSquared *= weightTransparent * weightTransparent;
      }
    }

    // Warn if the tdc signal has stopped during the run
    if ((events.back().pulseTime() +
         static_cast<int64_t>(events.back().tof() * 1000.)) >
        (tdc.back() + static_cast<int64_t>(period * 2)))
      g_log.warning("Events occurred long after last TDC.");

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorelliCrossCorrelate.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using Types::Core::DateAndTime;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorelliCrossCorrelate)

/** Initialize the algorithm's properties.
 */
void CorelliCrossCorrelate::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
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

  // Must include the chopper4 MotorSpeed.
  else if (!inputWS->run().hasProperty("BL9:Chop:Skf4:MotorSpeed"))
    errors["InputWorkspace"] = "Workspace is missing chopper4 Motor Speed.";

  // Check if input workspace is sorted.
  else if (inputWS->getSortType() == UNSORTED)
    errors["InputWorkspace"] = "The workspace needs to be a sorted.";

  // Check event type for pulse times
  else if (inputWS->getEventType() == WEIGHTED_NOTIME)
    errors["InputWorkspace"] = "This workspace has no pulse time information.";

  return errors;
}

/** Execute the algorithm.
 */
void CorelliCrossCorrelate::exec() {
  inputWS = getProperty("InputWorkspace");
  outputWS = getProperty("OutputWorkspace");

  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
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
  float weightAbsorbing = static_cast<float>(-dutyCycle / (1.0 - dutyCycle));
  g_log.information() << "dutyCycle = " << dutyCycle
                      << " weightTransparent = 1.0"
                      << " weightAbsorbing = " << weightAbsorbing << "\n";

  // Read in the TDC timings for the correlation chopper and apply the timing
  // offset.
  auto chopperTdc = dynamic_cast<ITimeSeriesProperty *>(
      inputWS->run().getLogData("chopper4_TDC"));
  if (!chopperTdc) {
    throw std::runtime_error("chopper4_TDC not found");
  }
  std::vector<DateAndTime> tdc = chopperTdc->timesAsVector();

  int offset_int = getProperty("TimingOffset");
  const int64_t offset = static_cast<int64_t>(offset_int);
  for (auto &timing : tdc)
    timing += offset;

  // Determine period from chopper frequency.
  auto motorSpeed = dynamic_cast<TimeSeriesProperty<double> *>(
      inputWS->run().getProperty("BL9:Chop:Skf4:MotorSpeed"));
  if (!motorSpeed) {
    throw Exception::NotFoundError(
        "Could not find a log value for the motor speed",
        "BL9:Chop:Skf4:MotorSpeed");
  }
  double period = 1e9 / static_cast<double>(motorSpeed->timeAverageValue());
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
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto &evlist = outputWS->getSpectrum(i);

    // Switch to weighted if needed.
    if (evlist.getEventType() == TOF)
      evlist.switchTo(WEIGHTED);

    std::vector<WeightedEvent> &events = evlist.getWeightedEvents();

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
        distanceSourceToSample + spectrumInfo.l2(i);
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

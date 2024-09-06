// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemovePromptPulse.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using std::string;
namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemovePromptPulse)

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------

const string RemovePromptPulse::name() const { return "RemovePromptPulse"; }

int RemovePromptPulse::version() const { return 1; }

const string RemovePromptPulse::category() const { return "CorrectionFunctions\\BackgroundCorrections"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemovePromptPulse::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<WorkspaceUnitValidator>("TOF")),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  auto validator = std::make_shared<BoundedValidator<double>>();
  validator->setLower(0.0);
  declareProperty("Width", Mantid::EMPTY_DBL(), validator,
                  "The width of the time of flight (in microseconds) to remove from the data.");
  declareProperty("Frequency", Mantid::EMPTY_DBL(), validator,
                  "The frequency of the source (in Hz) used to calculate the minimum time of flight to filter.");
  declareProperty(
      "TMin", Mantid::EMPTY_DBL(),
      "Minimum time of flight. "
      "Execution will be faster if this is specified, but the value will be determined from the workspace if not.");
  declareProperty(
      "TMax", Mantid::EMPTY_DBL(),
      "Minimum time of flight. "
      "Execution will be faster if this is specified, but the value will be determined from the workspace if not.");
}

//----------------------------------------------------------------------------------------------
namespace { // anonymous namespace begin
double getMedian(const API::Run &run, const std::string &name) {

  if (!run.hasProperty(name)) {
    return Mantid::EMPTY_DBL();
  }
  try {
    return run.getPropertyAsSingleValue(name, Kernel::Math::Median);
  } catch (const std::invalid_argument &) {
    return Mantid::EMPTY_DBL(); // maybe one of the other names will work
  }
}
} // namespace

void RemovePromptPulse::getTofRange(const MatrixWorkspace_const_sptr &wksp, double &tmin, double &tmax) {
  const auto timerStart = std::chrono::high_resolution_clock::now();

  // first get the values from the properties
  tmin = getProperty("TMin");
  tmax = getProperty("TMax");

  // only get the values that are not specified
  const bool findTmin = isEmpty(tmin);
  const bool findTmax = isEmpty(tmax);

  if (findTmin && findTmax) {
    if (const auto eventWksp = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(wksp)) {
      eventWksp->getEventXMinMax(tmin, tmax);
    } else {
      wksp->getXMinMax(tmin, tmax);
    }
  } else if (findTmin) {
    if (const auto eventWksp = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(wksp)) {
      tmin = eventWksp->getEventXMin();
    } else {
      tmin = wksp->getXMin();
    }
  } else if (findTmax) {
    if (const auto eventWksp = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(wksp)) {
      tmax = eventWksp->getEventXMax();
    } else {
      tmax = wksp->getXMax();
    }
  }
  // the fall-through case is to use the properties for both which was set at the top of the function

  addTimer("getTofRange", timerStart, std::chrono::high_resolution_clock::now());
}

/** Execute the algorithm.
 */
void RemovePromptPulse::exec() {
  // verify there is a width parameter specified
  double width = this->getProperty("Width");
  if (this->isEmpty(width)) {
    throw std::runtime_error("Failed to specify \'Width\' parameter");
  }

  // need the input workspace in general
  API::MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");

  // get the frequency
  double frequency = this->getProperty("Frequency");
  if (this->isEmpty(frequency)) { // it wasn't specified so try divination
    frequency = this->getFrequency(inputWS->run());
    if (this->isEmpty(frequency)) {
      throw std::runtime_error("Failed to determine the frequency");
    }
  }
  g_log.information() << "Using frequency of " << frequency << "Hz\n";
  double period = 1000000. / frequency; // period in microseconds

  // determine the overall tof window for the data
  double tmin;
  double tmax;
  getTofRange(inputWS, tmin, tmax);
  g_log.information() << "Data tmin=" << tmin << ", tmax=" << tmax << ", period=" << period << " microseconds\n";

  // calculate the times for the prompt pulse
  std::vector<double> pulseTimes = this->calculatePulseTimes(tmin, tmax, period, width);
  if (pulseTimes.empty()) {
    g_log.notice() << "Not applying filter since prompt pulse is not in data "
                      "range (period = "
                   << period << ")\n";
    setProperty("OutputWorkspace", std::const_pointer_cast<MatrixWorkspace>(inputWS));
    return;
  }
  g_log.information() << "Calculated prompt pulses at ";
  for (double pulseTime : pulseTimes)
    g_log.information() << pulseTime << " ";
  g_log.information() << " microseconds\n";

  // loop through each prompt pulse
  MatrixWorkspace_sptr outputWS;
  auto algo = createChildAlgorithm("MaskBins");
  for (const double &pulseTime : pulseTimes) {
    const double right = pulseTime + width;

    g_log.notice() << "Filtering tmin=" << pulseTime << ", tmax=" << right << " microseconds\n";

    // run maskbins to do the work on the first prompt pulse
    if (outputWS) {
      algo->setProperty<MatrixWorkspace_sptr>("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(outputWS));
    } else { // should only be first time
      algo->setProperty<MatrixWorkspace_sptr>("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(inputWS));
      outputWS = this->getProperty("OutputWorkspace");
    }
    // always write to correct output workspace
    algo->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    algo->setProperty<double>("XMin", pulseTime);
    algo->setProperty<double>("XMax", right);
    algo->executeAsChildAlg();

    // copy over the output workspace
    outputWS = algo->getProperty("OutputWorkspace");
  }

  setProperty("OutputWorkspace", outputWS);
}

double RemovePromptPulse::getFrequency(const API::Run &run) {
  double candidate;

  candidate = getMedian(run, "Frequency");
  if (!(this->isEmpty(candidate)))
    return candidate;

  candidate = getMedian(run, "frequency");
  if (!(this->isEmpty(candidate)))
    return candidate;

  candidate = getMedian(run, "FREQUENCY");
  if (!(this->isEmpty(candidate)))
    return candidate;

  // give up
  return Mantid::EMPTY_DBL();
}

/**
 * Calculate when all prompt pulses might be between the supplied times.
 * @param tmin The minimum time-of-flight measured.
 * @param tmax The maximum time-of-flight measured.
 * @param period The accelerator period.
 * @param width The width of the time of flight (in microseconds) to remove from the data.
 * @return A vector of all prompt pulse times possible within the time-of-flight
 * range.
 */
std::vector<double> RemovePromptPulse::calculatePulseTimes(const double tmin, const double tmax, const double period,
                                                           const double width) {
  std::vector<double> times;
  double time = 0.;
  // zero pulse should be taken into account
  if (tmin > 0 && tmin < width)
    times.emplace_back(time);
  // find when the first prompt pulse would be
  while (time < tmin)
    time += period;
  // calculate all times possible
  while (time < tmax) {
    times.emplace_back(time);
    time += period;
  }

  return times;
}
} // namespace Mantid::Algorithms

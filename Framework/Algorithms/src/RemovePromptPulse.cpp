// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemovePromptPulse.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using std::string;
namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemovePromptPulse)

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------

const string RemovePromptPulse::name() const { return "RemovePromptPulse"; }

int RemovePromptPulse::version() const { return 1; }

const string RemovePromptPulse::category() const {
  return "CorrectionFunctions\\BackgroundCorrections";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemovePromptPulse::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");

  auto validator = boost::make_shared<BoundedValidator<double>>();
  validator->setLower(0.0);
  declareProperty("Width", Mantid::EMPTY_DBL(), validator,
                  "The width of the time of flight (in microseconds) to remove "
                  "from the data.");
  declareProperty("Frequency", Mantid::EMPTY_DBL(), validator,
                  "The frequency of the source (in Hz) used to calculate the "
                  "minimum time of flight to filter.");
}

//----------------------------------------------------------------------------------------------
namespace { // anonymous namespace begin
double getMedian(const API::Run &run, const std::string &name) {

  if (!run.hasProperty(name)) {
    return Mantid::EMPTY_DBL();
  }
  auto *log =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(run.getLogData(name));
  if (!log)
    return Mantid::EMPTY_DBL();

  Kernel::TimeSeriesPropertyStatistics stats = log->getStatistics();
  return stats.median;
}

void getTofRange(MatrixWorkspace_const_sptr wksp, double &tmin, double &tmax) {
  DataObjects::EventWorkspace_const_sptr eventWksp =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(wksp);
  if (eventWksp == nullptr) {
    wksp->getXMinMax(tmin, tmax);
  } else {
    eventWksp->getEventXMinMax(tmin, tmax);
  }
}
} // namespace

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
  if (this->isEmpty(frequency)) // it wasn't specified so try divination
  {
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
  g_log.information() << "Data tmin=" << tmin << ", tmax=" << tmax
                      << ", period=" << period << " microseconds\n";

  // calculate the times for the prompt pulse
  std::vector<double> pulseTimes =
      this->calculatePulseTimes(tmin, tmax, period);
  if (pulseTimes.empty()) {
    g_log.notice() << "Not applying filter since prompt pulse is not in data "
                      "range (period = "
                   << period << ")\n";
    setProperty("OutputWorkspace",
                boost::const_pointer_cast<MatrixWorkspace>(inputWS));
    return;
  }
  g_log.information() << "Calculated prompt pulses at ";
  for (double pulseTime : pulseTimes)
    g_log.information() << pulseTime << " ";
  g_log.information() << " microseconds\n";

  MatrixWorkspace_sptr outputWS;
  for (double &pulseTime : pulseTimes) {
    double right = pulseTime + width;

    g_log.notice() << "Filtering tmin=" << pulseTime << ", tmax=" << right
                   << " microseconds\n";

    // run maskbins to do the work on the first prompt pulse
    IAlgorithm_sptr algo = this->createChildAlgorithm("MaskBins");
    if (outputWS) {
      algo->setProperty<MatrixWorkspace_sptr>(
          "InputWorkspace",
          boost::const_pointer_cast<MatrixWorkspace>(outputWS));
    } else { // should only be first time
      algo->setProperty<MatrixWorkspace_sptr>(
          "InputWorkspace",
          boost::const_pointer_cast<MatrixWorkspace>(inputWS));
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
 * @return A vector of all prompt pulse times possible within the time-of-flight
 * range.
 */
std::vector<double>
RemovePromptPulse::calculatePulseTimes(const double tmin, const double tmax,
                                       const double period) {
  std::vector<double> times;
  double time = 0.;

  // find when the first prompt pulse would be
  while (time < tmin)
    time += period;

  // calculate all times possible
  while (time < tmax) {
    times.push_back(time);
    time += period;
  }

  return times;
}
} // namespace Algorithms
} // namespace Mantid

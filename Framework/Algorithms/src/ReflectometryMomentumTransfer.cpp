// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ReflectometryMomentumTransfer.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/ReflectometryBeamStatistics.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/math/special_functions/pow.hpp>

namespace {
/// A private namespace for algorithm's property names.
namespace Prop {
static const std::string CHOPPER_OPENING{"ChopperOpening"};
static const std::string CHOPPER_PAIR_DIST{"ChopperPairDistance"};
static const std::string CHOPPER_RADIUS{"ChopperRadius"};
static const std::string CHOPPER_SPEED{"ChopperSpeed"};
static const std::string DETECTOR_RESOLUTION{"DetectorResolution"};
static const std::string INPUT_WS{"InputWorkspace"};
static const std::string OUTPUT_WS{"OutputWorkspace"};
static const std::string PIXEL_SIZE{"PixelSize"};
static const std::string REFLECTED_FOREGROUND{"ReflectedForeground"};
static const std::string SLIT1_NAME{"FirstSlitName"};
static const std::string SLIT1_SIZE_LOG{"FirstSlitSizeSampleLog"};
static const std::string SLIT2_NAME{"SecondSlitName"};
static const std::string SLIT2_SIZE_LOG{"SecondSlitSizeSampleLog"};
static const std::string SUM_TYPE{"SummationType"};
static const std::string TOF_CHANNEL_WIDTH{"TOFChannelWidth"};
} // namespace Prop

/// Choices for the SUM_TYPE property.
namespace SumTypeChoice {
static const std::string LAMBDA{"SumInLambda"};
static const std::string Q{"SumInQ"};
} // namespace SumTypeChoice

/// A conversion factor from e.g. slit opening to FWHM of Gaussian equivalent.
constexpr double FWHM_GAUSSIAN_EQUIVALENT{0.68};

/**
 * Returns a double value from sample logs.
 * @param run a Run object.
 * @param entry name of the sample log entry
 * @return the value of the log entry
 * @throws NotFoundError if the log does not exist
 * @throws runtime_error if the log value cannot be converted to double
 */
double fromLogs(const Mantid::API::Run &run, const std::string &entry) {
  if (!run.hasProperty(entry)) {
    throw Mantid::Kernel::Exception::NotFoundError(
        "Could not find sample log entry", entry);
  }
  try {
    return run.getPropertyValueAsType<double>(entry);
  } catch (std::invalid_argument &) {
    throw std::runtime_error("Could not parse a number from log entry " +
                             entry);
  }
}
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryMomentumTransfer)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometryMomentumTransfer::name() const {
  return "ReflectometryMomentumTransfer";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryMomentumTransfer::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryMomentumTransfer::category() const {
  return "ILL\\Reflectometry;Reflectometry";
}

/// Return a vector of related algorithms.
const std::vector<std::string> ReflectometryMomentumTransfer::seeAlso() const {
  return {"ReflectometryBeamStatistics", "ConvertToReflectometryQ",
          "ConvertUnits"};
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryMomentumTransfer::summary() const {
  return "Convert wavelength to momentum transfer and calculate the Qz "
         "resolution for reflectometers at continuous beam sources.";
}

/** Initialize the algorithm's properties.
 */
void ReflectometryMomentumTransfer::init() {
  auto inWavelength =
      boost::make_shared<API::WorkspaceUnitValidator>("Wavelength");
  auto twoElementArray =
      boost::make_shared<Kernel::ArrayLengthValidator<int>>(2);
  auto mandatoryDouble =
      boost::make_shared<Kernel::MandatoryValidator<double>>();
  auto positiveDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLowerExclusive(0.);
  auto mandatoryPositiveDouble =
      boost::make_shared<Kernel::CompositeValidator>();
  mandatoryPositiveDouble->add(mandatoryDouble);
  mandatoryPositiveDouble->add(positiveDouble);
  auto mandatoryString =
      boost::make_shared<Kernel::MandatoryValidator<std::string>>();
  std::vector<std::string> sumTypes(2);
  sumTypes.front() = SumTypeChoice::LAMBDA;
  sumTypes.back() = SumTypeChoice::Q;
  auto acceptableSumTypes =
      boost::make_shared<Kernel::ListValidator<std::string>>(sumTypes);
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input, inWavelength),
      "A reflectivity workspace with X units in wavelength.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "The input workspace with X units converted to Q and DX values set to "
      "the Q resolution.");
  declareProperty(Prop::SUM_TYPE, SumTypeChoice::LAMBDA, acceptableSumTypes,
                  "The type of summation performed for the input workspace.");
  declareProperty(Prop::REFLECTED_FOREGROUND, std::vector<int>(),
                  twoElementArray,
                  "A two element list [start, end] defining the reflected beam "
                  "foreground region in workspace indices.");
  declareProperty(Prop::PIXEL_SIZE, EMPTY_DBL(), mandatoryPositiveDouble,
                  "Detector pixel size, in meters.");
  declareProperty(Prop::DETECTOR_RESOLUTION, EMPTY_DBL(),
                  mandatoryPositiveDouble,
                  "Detector pixel resolution, in meters.");
  declareProperty(Prop::CHOPPER_SPEED, EMPTY_DBL(), mandatoryPositiveDouble,
                  "Chopper speed, in rpm.");
  declareProperty(Prop::CHOPPER_OPENING, EMPTY_DBL(), mandatoryPositiveDouble,
                  "The opening angle between the two choppers, in degrees.");
  declareProperty(Prop::CHOPPER_RADIUS, EMPTY_DBL(), mandatoryPositiveDouble,
                  "Chopper radius, in meters.");
  declareProperty(Prop::CHOPPER_PAIR_DIST, EMPTY_DBL(), mandatoryPositiveDouble,
                  "The gap between two choppers, in meters.");
  declareProperty(Prop::SLIT1_NAME, "", mandatoryString,
                  "Name of the first slit component.");
  declareProperty(Prop::SLIT1_SIZE_LOG, "", mandatoryString,
                  "The sample log entry for the first slit opening.");
  declareProperty(Prop::SLIT2_NAME, "", mandatoryString,
                  "Name of the second slit component.");
  declareProperty(Prop::SLIT2_SIZE_LOG, "", mandatoryString,
                  "The sample log entry for the second slit opening.");
  declareProperty(Prop::TOF_CHANNEL_WIDTH, EMPTY_DBL(), mandatoryPositiveDouble,
                  "TOF bin width, in microseconds.");
}

/// Validates the algorithm's input properties.
std::map<std::string, std::string>
ReflectometryMomentumTransfer::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr inWS = getProperty(Prop::INPUT_WS);
  if (inWS->getNumberHistograms() != 1) {
    issues[Prop::INPUT_WS] = "Expected a workspace with a single histogram.";
  } else {
    const auto &spectrumInfo = inWS->spectrumInfo();
    if (spectrumInfo.isMonitor(0)) {
      issues[Prop::INPUT_WS] = "The only histogram is marked as a monitor.";
    }
    if (spectrumInfo.isMasked(0)) {
      issues[Prop::INPUT_WS] = "The only histogram is masked.";
    }
  }
  const std::string slit1Name = getProperty(Prop::SLIT1_NAME);
  auto instrument = inWS->getInstrument();
  auto slit = instrument->getComponentByName(slit1Name);
  if (!slit) {
    issues[Prop::SLIT1_NAME] =
        "No component called '" + slit1Name + "' found in " + Prop::INPUT_WS;
  }
  const std::string slit2Name = getProperty(Prop::SLIT1_NAME);
  slit = instrument->getComponentByName(slit2Name);
  if (!slit) {
    issues[Prop::SLIT2_NAME] =
        "No component called '" + slit2Name + "' found in " + Prop::INPUT_WS;
  }
  return issues;
}

/** Execute the algorithm.
 */
void ReflectometryMomentumTransfer::exec() {
  using namespace boost::math;
  API::MatrixWorkspace_const_sptr inWS = getProperty(Prop::INPUT_WS);
  auto setup = createSetup(*inWS);
  auto beam = createBeamStatistics(*inWS);
  API::MatrixWorkspace_sptr outWS = inWS->clone();
  convertToMomentumTransfer(outWS);
  addResolutionDX(*inWS, *outWS, setup, beam);
  setProperty(Prop::OUTPUT_WS, outWS);
}

/**
 * Adds the Q resolution to outWS as the DX values
 * @param inWS the input workspace
 * @param outWS the workspace to write the resolutions to
 * @param setup a setup object
 * @param beam a beam statistics object
 */
void ReflectometryMomentumTransfer::addResolutionDX(
    const API::MatrixWorkspace &inWS, API::MatrixWorkspace &outWS,
    const Setup &setup, const Beam &beam) {
  const auto deltaThetaSq = angularResolutionSquared(inWS, setup, beam);
  const auto &wavelengths = inWS.points(0);
  const auto &qs = outWS.points(0);
  auto dx = Kernel::make_cow<HistogramData::HistogramDx>(qs.size(), 0);
  outWS.setSharedDx(0, dx);
  auto &resolutions = outWS.mutableDx(0);
  for (size_t i = 0; i < wavelengths.size(); ++i) {
    const auto wavelength = wavelengths[i] * 1e-10;
    const auto deltaLambdaSq = wavelengthResolutionSquared(setup, wavelength);
    // q is inversely proportional to wavelength but sorted in ascending
    // order.
    const auto qIndex = qs.size() - i - 1;
    resolutions[qIndex] = qs[qIndex] * std::sqrt(deltaLambdaSq + deltaThetaSq);
  }
}

/** Calculate the squared angular resolution.
 *
 * @param ws the reflectivity workspace
 * @param setup a setup object for the reflected beam
 * @param beam a beam statistics object
 * @return the squared fractional angular resolution
 */
double ReflectometryMomentumTransfer::angularResolutionSquared(
    const API::MatrixWorkspace &ws, const Setup &setup, const Beam &beam) {
  using namespace boost::math;
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto l2 = spectrumInfo.l2(0);
  const auto braggAngle = 0.5 * spectrumInfo.twoTheta(0);
  if (setup.sumType == SumType::Q) {
    if (beam.sampleWaviness > 0) {
      // Bent sample resolution
      if (beam.firstSlitAngularSpread >= 2. * beam.sampleWaviness) {
        return (pow<2>(setup.detectorResolution /
                       (setup.slit2SampleDistance + l2)) +
                pow<2>(beam.secondSlitAngularSpread) +
                pow<2>(beam.sampleWaviness)) /
               pow<2>(braggAngle);
      } else {
        return (pow<2>(setup.detectorResolution / 2. /
                       (setup.slit2SampleDistance + l2)) +
                pow<2>(beam.secondSlitAngularSpread) +
                pow<2>(beam.firstSlitAngularSpread)) /
               pow<2>(braggAngle);
      }
    } else {
      if (beam.firstSlitAngularSpread > setup.detectorResolution / l2) {
        // Divergent beam resolution
        return (pow<2>(setup.detectorResolution /
                       (setup.slit2SampleDistance + l2)) +
                pow<2>(beam.secondSlitAngularSpread)) /
               pow<2>(braggAngle);
      } else {
        // Detector resolution is worse than the incoming divergence for a flat
        // sample.
        return (pow<2>(beam.incidentAngularSpread) +
                pow<2>(setup.detectorResolution /
                       (setup.slit2SampleDistance + l2))) /
               pow<2>(braggAngle);
      }
    }
  } else { // SumType::LAMBDA
    const auto angularResolution =
        (pow<2>(beam.incidentAngularSpread) + pow<2>(beam.sampleWaviness)) /
        pow<2>(braggAngle);
    // In case foreground width is smaller than incoming angular resolution
    const auto foregroundWidth =
        static_cast<double>(setup.foregroundEnd - setup.foregroundStart + 1) *
        setup.pixelSize;
    const auto foregroundWidthLimited =
        pow<2>(FWHM_GAUSSIAN_EQUIVALENT) *
        (pow<2>(foregroundWidth) + pow<2>(setup.slit2Size)) /
        pow<2>(l2 * braggAngle);
    return std::min(angularResolution, foregroundWidthLimited);
  }
}

/** Convert a workspace's X units to momentum transfer in-place.
 *
 * @param ws a workspace to convert.
 */
void ReflectometryMomentumTransfer::convertToMomentumTransfer(
    API::MatrixWorkspace_sptr &ws) {
  auto convert = createChildAlgorithm("ConvertUnits");
  convert->setProperty("InputWorkspace", ws);
  convert->setProperty("OutputWorkspace", "unused_for_child");
  convert->setProperty("Target", "MomentumTransfer");
  convert->execute();
  ws = convert->getProperty("OutputWorkspace");
}

/**
 * Creates a beam statistics object from sample logs
 * @param ws a workspace
 * @return beam statistics
 */
const ReflectometryMomentumTransfer::Beam
ReflectometryMomentumTransfer::createBeamStatistics(
    const API::MatrixWorkspace &ws) {
  Beam b;
  const auto &run = ws.run();
  b.incidentAngularSpread = fromLogs(
      run, ReflectometryBeamStatistics::LogEntry::INCIDENT_ANGULAR_SPREAD);
  b.firstSlitAngularSpread = fromLogs(
      run, ReflectometryBeamStatistics::LogEntry::FIRST_SLIT_ANGULAR_SPREAD);
  b.secondSlitAngularSpread = fromLogs(
      run, ReflectometryBeamStatistics::LogEntry::SECOND_SLIT_ANGULAR_SPREAD);
  b.sampleWaviness =
      fromLogs(run, ReflectometryBeamStatistics::LogEntry::SAMPLE_WAVINESS);
  return b;
}

/** Generate a setup for the reflected beam experiment.
 *
 * @param ws the reflectivity workspace
 * @return a setup object
 */
const ReflectometryMomentumTransfer::Setup
ReflectometryMomentumTransfer::createSetup(const API::MatrixWorkspace &ws) {
  Setup s;
  s.chopperOpening = static_cast<double>(getProperty(Prop::CHOPPER_OPENING)) *
                     Geometry::deg2rad;
  s.chopperPairDistance = getProperty(Prop::CHOPPER_PAIR_DIST);
  s.chopperPeriod =
      1. / (static_cast<double>(getProperty(Prop::CHOPPER_SPEED)) / 60.);
  s.chopperRadius = getProperty(Prop::CHOPPER_RADIUS);
  s.detectorResolution = getProperty(Prop::DETECTOR_RESOLUTION);
  std::vector<int> foreground = getProperty(Prop::REFLECTED_FOREGROUND);
  auto lowPixel = static_cast<size_t>(foreground.front());
  auto highPixel = static_cast<size_t>(foreground.back());
  s.foregroundStart = std::min(lowPixel, highPixel);
  s.foregroundEnd = std::max(lowPixel, highPixel);
  const auto &spectrumInfo = ws.spectrumInfo();
  s.l1 = spectrumInfo.l1();
  s.l2 = spectrumInfo.l2(0);
  s.pixelSize = getProperty(Prop::PIXEL_SIZE);
  s.slit1Slit2Distance = interslitDistance(ws);
  const std::string slit1SizeEntry = getProperty(Prop::SLIT1_SIZE_LOG);
  s.slit1Size = slitSize(ws, slit1SizeEntry);
  const std::string slit2Name = getProperty(Prop::SLIT2_NAME);
  auto instrument = ws.getInstrument();
  auto slit2 = instrument->getComponentByName(slit2Name);
  const auto samplePos = spectrumInfo.samplePosition();
  s.slit2SampleDistance = (slit2->getPos() - samplePos).norm();
  const std::string slit2SizeEntry = getProperty(Prop::SLIT2_SIZE_LOG);
  s.slit2Size = slitSize(ws, slit2SizeEntry);
  const std::string sumType = getProperty(Prop::SUM_TYPE);
  s.sumType = sumType == SumTypeChoice::LAMBDA ? SumType::LAMBDA : SumType::Q;
  s.tofChannelWidth =
      static_cast<double>(getProperty(Prop::TOF_CHANNEL_WIDTH)) * 1e-6;
  return s;
}

/** Give the gap between the two slits, in meters.
 *
 * @param ws the reflectivity workspace
 * @return the slit gap, in meters
 */
double ReflectometryMomentumTransfer::interslitDistance(
    const API::MatrixWorkspace &ws) {
  const std::string slit1Name = getProperty(Prop::SLIT1_NAME);
  const std::string slit2Name = getProperty(Prop::SLIT2_NAME);
  auto instrument = ws.getInstrument();
  return ReflectometryBeamStatistics::slitSeparation(instrument, slit1Name,
                                                     slit2Name);
}

/** Read the slit size from samle logs.
 *
 * @param ws workspace to investigate
 * @param logEntry name of the slit opening sample log
 * @return the slit opening, in meters
 */
double ReflectometryMomentumTransfer::slitSize(const API::MatrixWorkspace &ws,
                                               const std::string &logEntry) {
  auto &run = ws.run();
  const double opening = run.getPropertyValueAsType<double>(logEntry);
  const auto &units = run.getProperty(logEntry)->units();
  if (units.empty()) {
    m_log.warning() << "Slit opening entry " << logEntry
                    << " has no unit. Assuming meters.\n";
    return opening;
  } else if (units == "m") {
    return opening;
  } else if (units == "mm") {
    return opening * 1e-3;
  } else {
    m_log.warning() << "Slit opening entry " << logEntry
                    << " has an unknown unit. Assuming meters.\n";
    return opening;
  }
}

/** Calculate the squared resolution due to wavelength variance.
 *
 * @param setup reflected beam setup
 * @param wavelength wavelength, in meters
 * @return the fractional resolution squared
 */
double ReflectometryMomentumTransfer::wavelengthResolutionSquared(
    const Setup &setup, const double wavelength) {
  // err_res in COSMOS
  using namespace boost::math;
  using namespace PhysicalConstants;
  const auto flightDistance = setup.l1 + setup.l2;
  const auto chopperResolution =
      setup.chopperPairDistance + h * setup.chopperOpening *
                                      setup.chopperPeriod /
                                      (2. * M_PI * NeutronMass * wavelength);
  const auto detectorResolution =
      h * setup.tofChannelWidth / (NeutronMass * wavelength);
  const auto partialResolution =
      0.49 *
      (3. * pow<2>(chopperResolution) + pow<2>(detectorResolution) +
       3. * chopperResolution * detectorResolution) /
      (2. * chopperResolution + detectorResolution) / flightDistance;
  const auto flightDistRatio =
      (setup.l1 - setup.slit2SampleDistance) / setup.slit1Slit2Distance;
  const auto a =
      flightDistRatio * (setup.slit1Size + setup.slit2Size) + setup.slit1Size;
  const auto b = flightDistRatio * std::abs(setup.slit1Size - setup.slit2Size) +
                 setup.slit1Size;
  const auto wavelengthSmearing =
      0.49 * (pow<3>(a) - pow<3>(b)) / (pow<2>(a) - pow<2>(b));
  const auto widthResolution = wavelengthSmearing * setup.chopperPeriod /
                               (2. * M_PI * setup.chopperRadius) * h /
                               (NeutronMass * wavelength * flightDistance);
  return pow<2>(partialResolution) + pow<2>(widthResolution);
}

} // namespace Algorithms
} // namespace Mantid

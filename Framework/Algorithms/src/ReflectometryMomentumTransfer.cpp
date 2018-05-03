#include "MantidAlgorithms/ReflectometryMomentumTransfer.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayLengthValidator.h"
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
static const std::string DIRECT_BEAM_WS{"DirectBeamWorkspace"};
static const std::string DIRECT_FOREGROUND{"DirectForeground"};
static const std::string INPUT_WS{"InputWorkspace"};
static const std::string OUTPUT_WS{"OutputWorkspace"};
static const std::string PIXEL_SIZE{"PixelSize"};
static const std::string POLARIZED{"Polarized"};
static const std::string REFLECTED_BEAM_WS{"ReflectedBeamWorkspace"};
static const std::string REFLECTED_FOREGROUND{"ReflectedForeground"};
static const std::string SLIT1_NAME{"Slit1Name"};
static const std::string SLIT1_SIZE_LOG{"Slit1SizeSampleLog"};
static const std::string SLIT2_NAME{"Slit2Name"};
static const std::string SLIT2_SIZE_LOG{"Slit2SizeSampleLog"};
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

/// Convert degrees to radians.
constexpr double inRad(const double a) noexcept { return a / 180. * M_PI; }
} // namespace

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

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
  auto mandatoryString =
      boost::make_shared<Kernel::MandatoryValidator<std::string>>();
  std::vector<std::string> sumTypes(2);
  sumTypes.front() = SumTypeChoice::LAMBDA;
  sumTypes.back() = SumTypeChoice::Q;
  auto acceptableSumTypes =
      boost::make_shared<Kernel::ListValidator<std::string>>(sumTypes);
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Direction::Input, inWavelength),
      "A reflectivity workspace in wavelenght.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Direction::Output, inWavelength),
      "The input workspace with DX values set to the Qz resolution.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::REFLECTED_BEAM_WS, "", Direction::Input, inWavelength),
      "A reflected beam workspace in wavelength.");
  declareProperty(Prop::REFLECTED_FOREGROUND, std::vector<int>(),
                  twoElementArray,
                  "A two element list [start, end] defining the reflected beam "
                  "foreground region in workspace indices.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::DIRECT_BEAM_WS, "", Direction::Input),
      "A direct beam workspace in wavelength.");
  declareProperty(Prop::DIRECT_FOREGROUND, std::vector<int>(), twoElementArray,
                  "A two element list [start, end] defining the direct beam "
                  "foreground region in workspace indices.");
  declareProperty(Prop::SUM_TYPE, SumTypeChoice::LAMBDA, acceptableSumTypes,
                  "The type of summation performed for the input workspace.");
  declareProperty(Prop::POLARIZED, false,
                  "True if the input workspace is part of polarization "
                  "analysis experiment, false otherwise.");
  declareProperty(Prop::PIXEL_SIZE, EMPTY_DBL(), mandatoryDouble,
                  "Detector pixel size, in meters.");
  declareProperty(Prop::DETECTOR_RESOLUTION, EMPTY_DBL(), mandatoryDouble,
                  "Detector pixel resolution, in meters.");
  declareProperty(Prop::CHOPPER_SPEED, EMPTY_DBL(), mandatoryDouble,
                  "Chopper speed, in rpm.");
  declareProperty(Prop::CHOPPER_OPENING, EMPTY_DBL(), mandatoryDouble,
                  "The opening angle between the two choppers, in degrees.");
  declareProperty(Prop::CHOPPER_RADIUS, EMPTY_DBL(), mandatoryDouble,
                  "Chopper radius, in meters.");
  declareProperty(Prop::CHOPPER_PAIR_DIST, EMPTY_DBL(), mandatoryDouble,
                  "The gap between two choppers, in meters.");
  declareProperty(Prop::SLIT1_NAME, "", mandatoryString,
                  "Name of the first slit component.");
  declareProperty(Prop::SLIT1_SIZE_LOG, "", mandatoryString,
                  "The sample log entry for the first slit opening.");
  declareProperty(Prop::SLIT2_NAME, "", mandatoryString,
                  "Name of the second slit component.");
  declareProperty(Prop::SLIT2_SIZE_LOG, "", mandatoryString,
                  "The sample log entry for the second slit opening.");
  declareProperty(Prop::TOF_CHANNEL_WIDTH, EMPTY_DBL(), mandatoryDouble,
                  "TOF bin width, in microseconds.");
}

/** Execute the algorithm.
 */
void ReflectometryMomentumTransfer::exec() {
  using namespace boost::math;
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);
  API::MatrixWorkspace_sptr reflectedWS = getProperty(Prop::REFLECTED_BEAM_WS);
  API::MatrixWorkspace_sptr directWS = getProperty(Prop::DIRECT_BEAM_WS);
  auto setup = createSetup(*reflectedWS, *directWS);
  API::MatrixWorkspace_sptr outWS;
  outWS = inWS->clone();
  convertToMomentumTransfer(outWS);
  const auto beamFWHM =
      beamRMSVariation(reflectedWS, setup.foregroundStart, setup.foregroundEnd);
  const auto directBeamFWHM = beamRMSVariation(
      directWS, setup.directForegroundStart, setup.directForegroundEnd);
  const auto incidentFWHM = incidentAngularSpread(setup);
  const auto slit1FWHM = slit1AngularSpread(setup);
  const auto &spectrumInfo = inWS->spectrumInfo();
  const int64_t nHisto = static_cast<int64_t>(outWS->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *directWS, *outWS))
  for (int64_t i = 0; i < nHisto; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto wsIndex = static_cast<size_t>(i);
    const auto &wavelengths = inWS->points(wsIndex);
    const auto &qs = outWS->points(wsIndex);
    auto dx = Kernel::make_cow<HistogramData::HistogramDx>(qs.size(), 0);
    outWS->setSharedDx(wsIndex, dx);
    if (spectrumInfo.isMonitor(wsIndex) || spectrumInfo.isMasked(wsIndex)) {
      // Skip monitors & masked spectra, leave DX to zero.
      continue;
    }
    auto &resolutions = outWS->mutableDx(wsIndex);
    for (size_t i = 0; i < wavelengths.size(); ++i) {
      const auto wavelength = wavelengths[i] * 1e-10;
      const auto deltaLambdaSq =
          wavelengthResolutionSquared(*inWS, wsIndex, setup, wavelength);
      const auto deltaThetaSq =
          angularResolutionSquared(inWS, *directWS, wsIndex, setup, beamFWHM,
                                   directBeamFWHM, incidentFWHM, slit1FWHM);
      // q is inversely proportional to wavelength but sorted in ascending
      // order.
      const auto qIndex = qs.size() - i - 1;
      resolutions[qIndex] =
          qs[qIndex] * std::sqrt(deltaLambdaSq + deltaThetaSq);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty(Prop::OUTPUT_WS, outWS);
}

/** Calculate the squared angular resolution.
 *
 * @param ws the reflectivity workspace
 * @param directWS the reference direct beam workspace
 * @param wsIndex a workspace index to the reflectivity workspace
 * @param setup a setup object for the reflected beam
 * @param beamFWHM reflected beam angular FWHM
 * @param directBeamFWHM direct beam angular FWHM
 * @param incidentFWHM RMS spread of the incident beam
 * @param slit1FWHM RMS spread due to the first slit
 * @return the squared fractional angular resolution
 */
double ReflectometryMomentumTransfer::angularResolutionSquared(
    API::MatrixWorkspace_sptr &ws, const API::MatrixWorkspace &directWS,
    const size_t wsIndex, const Setup &setup, const double beamFWHM,
    const double directBeamFWHM, const double incidentFWHM,
    const double slit1FWHM) {
  using namespace boost::math;
  const auto waviness = sampleWaviness(ws, directWS, wsIndex, setup, beamFWHM,
                                       directBeamFWHM, incidentFWHM);
  const auto slit2FWHM = slit2AngularSpread(*ws, wsIndex, setup);
  const auto &spectrumInfo = ws->spectrumInfo();
  const auto l2 = spectrumInfo.l2(wsIndex);
  const auto braggAngle = 0.5 * spectrumInfo.twoTheta(wsIndex);
  if (setup.sumType == SumType::Q) {
    if (waviness > 0) {
      if (slit1FWHM >= 2. * waviness) {
        return (pow<2>(setup.detectorResolution / l2) + pow<2>(slit2FWHM) +
                pow<2>(waviness)) /
               pow<2>(braggAngle);
      } else {
        return (pow<2>(setup.detectorResolution / 2. / l2) + pow<2>(slit2FWHM) +
                pow<2>(slit1FWHM)) /
               pow<2>(braggAngle);
      }
    } else {
      if (slit1FWHM > setup.detectorResolution / l2) {
        return (pow<2>(setup.detectorResolution / l2) + pow<2>(slit2FWHM)) /
               pow<2>(braggAngle);
      } else {
        const double incidentSpread = incidentFWHM;
        return (pow<2>(incidentSpread) +
                pow<2>(setup.detectorResolution / l2)) /
               pow<2>(braggAngle);
      }
    }
  } else { // SumType::LAMBDA
    const auto foregroundWidth =
        static_cast<double>(setup.foregroundEnd - setup.foregroundStart + 1) *
        setup.pixelSize;
    const auto foregroundWidthLimited =
        pow<2>(FWHM_GAUSSIAN_EQUIVALENT) *
        (pow<2>(foregroundWidth) + pow<2>(setup.slit2Size)) /
        pow<2>(l2 * braggAngle);
    double angularResolution;
    if (setup.polarized) {
      angularResolution = pow<2>(incidentFWHM / braggAngle);
    } else {
      angularResolution =
          (pow<2>(incidentFWHM) + pow<2>(waviness)) / pow<2>(braggAngle);
    }
    return std::min(angularResolution, foregroundWidthLimited);
  }
}

/** Calculate the FWHM of a beam (reflected or direct).
 *
 * @param ws a reference workspace
 * @param start foreground start workspace index
 * @param end foreground end workspace index
 * @return FWHM
 */
double ReflectometryMomentumTransfer::beamRMSVariation(
    API::MatrixWorkspace_sptr &ws, const size_t start, const size_t end) {
  // det_fwhm and detdb_fwhm in COSMOS
  using namespace boost::math;
  auto integrate = createChildAlgorithm("Integration");
  integrate->setProperty("InputWorkspace", ws);
  integrate->setProperty("OutputWorkspace", "unused_for_child");
  integrate->setProperty("StartWorkspaceIndex", static_cast<int>(start));
  integrate->setProperty("EndWorkspaceIndex", static_cast<int>(end));
  integrate->execute();
  API::MatrixWorkspace_const_sptr integratedWS =
      integrate->getProperty("OutputWorkspace");
  double sum{0.};
  double weighedSum{0.};
  std::vector<double> thetaDistribution(integratedWS->getNumberHistograms());
  for (size_t i = 0; i < thetaDistribution.size(); ++i) {
    const auto total = integratedWS->y(i).front();
    thetaDistribution[i] = total;
    sum += total;
    weighedSum += static_cast<double>(i) * total;
  }
  const double massCenter = weighedSum / sum;
  double variance{0.};
  for (size_t i = 0; i < thetaDistribution.size(); ++i) {
    variance +=
        thetaDistribution[i] * pow<2>(massCenter - static_cast<double>(i));
  }
  variance /= sum;
  const double pixelSize = getProperty(Prop::PIXEL_SIZE);
  return 2. * std::sqrt(2. * std::log(2.)) * pixelSize * std::sqrt(variance);
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

/** Calculate the detector angular resolution.
 *
 * @param ws a reflectivity workspace
 * @param wsIndex a workspace index to the reflectivity workspace
 * @param setup reflected beam setup
 * @param incidentFWHM spread of the incident beam
 * @return the da quantity
 */
double ReflectometryMomentumTransfer::detectorAngularResolution(
    const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup,
    const double incidentFWHM) {
  // da_det in COSMOS
  using namespace boost::math;
  const auto slitSizeRatio = setup.slit2Size / setup.slit1Size;
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto slit2Detector =
      setup.slit2SampleDistance + spectrumInfo.l2(wsIndex);
  const auto virtualSourceDist =
      slit2Detector +
      (slitSizeRatio * setup.slit1Slit2Distance) / (1. + slitSizeRatio);
  return std::sqrt(pow<2>(incidentFWHM * virtualSourceDist) +
                   pow<2>(setup.detectorResolution));
}

/** Generate a setup for the reflected beam experiment.
 *
 * @param ws the reflectivity workspace
 * @param directWS corresponding direct beam workspace
 * @return a setup object
 */
const ReflectometryMomentumTransfer::Setup
ReflectometryMomentumTransfer::createSetup(
    const API::MatrixWorkspace &ws, const API::MatrixWorkspace &directWS) {
  Setup s;
  s.chopperOpening = inRad(getProperty(Prop::CHOPPER_OPENING));
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
  foreground = getProperty(Prop::DIRECT_FOREGROUND);
  lowPixel = static_cast<size_t>(foreground.front());
  highPixel = static_cast<size_t>(foreground.back());
  s.directForegroundStart = std::min(lowPixel, highPixel);
  s.directForegroundEnd = std::max(lowPixel, highPixel);
  s.pixelSize = getProperty(Prop::PIXEL_SIZE);
  s.polarized = getProperty(Prop::POLARIZED);
  s.slit1Slit2Distance = interslitDistance(ws);
  const std::string slit1SizeEntry = getProperty(Prop::SLIT1_SIZE_LOG);
  s.slit1Size = slitSize(ws, slit1SizeEntry);
  s.slit1SizeDirectBeam = slitSize(directWS, slit1SizeEntry);
  const std::string slit2Name = getProperty(Prop::SLIT2_NAME);
  auto instrument = ws.getInstrument();
  auto slit2 = instrument->getComponentByName(slit2Name);
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto samplePos = spectrumInfo.samplePosition();
  s.slit2SampleDistance = (slit2->getPos() - samplePos).norm();
  const std::string slit2SizeEntry = getProperty(Prop::SLIT2_SIZE_LOG);
  s.slit2Size = slitSize(ws, slit2SizeEntry);
  s.slit2SizeDirectBeam = slitSize(directWS, slit2SizeEntry);
  const std::string sumType = getProperty(Prop::SUM_TYPE);
  s.sumType = sumType == SumTypeChoice::LAMBDA ? SumType::LAMBDA : SumType::Q;
  s.tofChannelWidth =
      static_cast<double>(getProperty(Prop::TOF_CHANNEL_WIDTH)) * 1e-6;
  return s;
}

/** Calculate the range of angles in the reflection plane determined by
 *  the collimation.
 *
 * @param setup a setup object
 * @return the incident FWHM
 */
double
ReflectometryMomentumTransfer::incidentAngularSpread(const Setup &setup) {
  // da in COSMOS
  using namespace boost::math;
  return FWHM_GAUSSIAN_EQUIVALENT *
         std::sqrt((pow<2>(setup.slit1Size) + pow<2>(setup.slit2Size))) /
         setup.slit1Slit2Distance;
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
  auto slit1 = instrument->getComponentByName(slit1Name);
  auto slit2 = instrument->getComponentByName(slit2Name);
  return (slit1->getPos() - slit2->getPos()).norm();
}

/** Calculate sample RMS waviness.
 *
 * @param ws the reflectivity workspace
 * @param directWS reference direct beam workspace
 * @param wsIndex workspace index to the reflectivity workspace
 * @param setup a setup object
 * @param beamFWHM reflected beam RMS variation
 * @param directBeamFWHM direct beam RMS variation
 * @param incidentFWHM incident beam angular spread
 * @return the waviness
 */
double ReflectometryMomentumTransfer::sampleWaviness(
    API::MatrixWorkspace_sptr &ws, const API::MatrixWorkspace &directWS,
    const size_t wsIndex, const Setup &setup, const double beamFWHM,
    const double directBeamFWHM, const double incidentFWHM) {
  // om_fwhm in COSMOS
  using namespace boost::math;
  const double slitSizeTolerance{0.00004};
  if (std::abs(setup.slit1Size - setup.slit1SizeDirectBeam) >=
          slitSizeTolerance ||
      std::abs(setup.slit2Size - setup.slit2SizeDirectBeam) >=
          slitSizeTolerance) {
    // Differing slit sizes branch from COSMOS.
    const double daDet =
        detectorAngularResolution(*ws, wsIndex, setup, incidentFWHM);
    if (beamFWHM - daDet >= 0) {
      const auto a = std::sqrt(pow<2>(beamFWHM) - pow<2>(daDet));
      if (a >= setup.pixelSize) {
        const auto directL2 = directWS.spectrumInfo().l2(wsIndex);
        return 0.5 * a / directL2;
      }
    }
  } else if (pow<2>(beamFWHM) - pow<2>(directBeamFWHM) >= 0) {
    const auto a = std::sqrt(pow<2>(beamFWHM) - pow<2>(directBeamFWHM));
    if (a >= setup.pixelSize) {
      const auto directL2 = directWS.spectrumInfo().l2(wsIndex);
      return 0.5 * a / directL2;
    }
  }
  return 0.;
}

/** Calculate the angular spread due to the first slit.
 *
 * @param setup a reflected beam setup object
 * @return the spread
 */
double ReflectometryMomentumTransfer::slit1AngularSpread(const Setup &setup) {
  // S2_fwhm in COSMOS
  return FWHM_GAUSSIAN_EQUIVALENT * setup.slit1Size / setup.slit1Slit2Distance;
}

/** Calculate the angular spread due to the second slit.
 *
 * @param ws the reflectivity workspace
 * @param wsIndex a workspace index to the reflectivity workspace
 * @param setup reflected beam setup
 * @return the spread
 */
double ReflectometryMomentumTransfer::slit2AngularSpread(
    const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup) {
  // s3_fwhm in COSMOS.
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto slit2Detector =
      setup.slit2SampleDistance + spectrumInfo.l2(wsIndex);
  return FWHM_GAUSSIAN_EQUIVALENT * setup.slit2Size / slit2Detector;
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
 * @param ws the reflectivity workspace
 * @param wsIndex a workspace index to the reflectivity workspace
 * @param setup reflected beam setup
 * @param wavelength wavelength, in meters
 * @return the fractional resolution squared
 */
double ReflectometryMomentumTransfer::wavelengthResolutionSquared(
    const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup,
    const double wavelength) {
  // err_res in COSMOS
  using namespace boost::math;
  using namespace PhysicalConstants;
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto l1 = spectrumInfo.l1();
  const auto l2 = spectrumInfo.l2(wsIndex);
  const auto flightDistance = l1 + l2;
  const auto chopperResolution = setup.chopperPairDistance +
                                 h * setup.chopperOpening *
                                     setup.chopperPeriod /
                                     (2. * M_PI * NeutronMass * wavelength);
  const auto detectorResolution =
      h * setup.tofChannelWidth / (NeutronMass * wavelength);
  const auto partialResolution =
      0.49 * (3. * pow<2>(chopperResolution) + pow<2>(detectorResolution) +
              3. * chopperResolution * detectorResolution) /
      (2. * chopperResolution + detectorResolution) / flightDistance;
  const auto flightDistRatio =
      (l1 - setup.slit2SampleDistance) / setup.slit1Slit2Distance;
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

#include "MantidAlgorithms/ReflectometryQResolution.h"

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
namespace Prop {
static const std::string CHOPPER_OPENING{"ChopperOpening"};
static const std::string CHOPPER_PAIR_DIST{"ChopperPairDistance"};
static const std::string CHOPPER_SPEED{"ChopperSpeed"};
static const std::string DETECTOR_RESOLUTION{"DetectorResolution"};
static const std::string DIRECT_BEAM_WS{"DirectBeamWorkspace"};
static const std::string FOREGROUND{"Foreground"};
static const std::string INPUT_WS{"InputWorkspace"};
static const std::string OUTPUT_WS{"OutputWorkspace"};
static const std::string PIXEL_SIZE{"PixelSize"};
static const std::string POLARIZED{"Polarized"};
static const std::string REFLECTED_BEAM_WS{"ReflectedBeamWorkspace"};
static const std::string SLIT1_NAME{"Slit1Name"};
static const std::string SLIT1_SIZE_LOG{"Slit1SizeSampleLog"};
static const std::string SLIT2_NAME{"Slit2Name"};
static const std::string SLIT2_SIZE_LOG{"Slit2SizeSampleLog"};
static const std::string SUM_TYPE{"SummationType"};
static const std::string TOF_CHANNEL_WIDTH{"TOFChannelWidth"};
}

namespace SumTypeChoice {
static const std::string LAMBDA{"SumInLambda"};
static const std::string Q{"SumInQ"};
}

constexpr double inRad(const double a) noexcept {
  return a / 180. * M_PI;
}
}

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryQResolution)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometryQResolution::name() const { return "ReflectometryQResolution"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryQResolution::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryQResolution::category() const {
  return "ILL\\Reflectometry;Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryQResolution::summary() const {
  return "Calculates the Qz resolution for reflectometers at continuous beam sources.";
}

/** Initialize the algorithm's properties.
 */
void ReflectometryQResolution::init() {
  auto inWavelength = boost::make_shared<API::WorkspaceUnitValidator>("Wavelength");
  auto twoElementArray = boost::make_shared<Kernel::ArrayLengthValidator<int>>(2);
  auto mandatoryDouble = boost::make_shared<Kernel::MandatoryValidator<double>>();
  auto mandatoryString = boost::make_shared<Kernel::MandatoryValidator<std::string>>();
  std::vector<std::string> sumTypes(2);
  sumTypes.front() = SumTypeChoice::LAMBDA;
  sumTypes.back() = SumTypeChoice::Q;
  auto acceptableSumTypes = boost::make_shared<Kernel::ListValidator<std::string>>(sumTypes);
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::INPUT_WS, "",
                                                             Direction::Input, inWavelength),
      "A reflectivity workspace in wavelenght.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUTPUT_WS, "",
                                                             Direction::Output, inWavelength),
      "The input workspace with DX values set to the Qz resolution.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::REFLECTED_BEAM_WS, "",
                                                             Direction::Input, inWavelength),
      "A reflected beam workspace in wavelength.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::DIRECT_BEAM_WS, "",
                                                             Direction::Input),
      "A direct beam workspace in wavelength.");
  declareProperty(Prop::FOREGROUND, std::vector<int>(), twoElementArray, "A list of three elements [low angle width, center, high angle width] defining the foreground region in pixels.");
  declareProperty(Prop::SUM_TYPE, SumTypeChoice::LAMBDA, acceptableSumTypes, "The type of summation performed for the input workspace.");
  declareProperty(Prop::POLARIZED, false, "True if the input workspace is part of polarization analysis experiment, false otherwise.");
  declareProperty(Prop::PIXEL_SIZE, EMPTY_DBL(), mandatoryDouble, "Detector pixel size, in meters.");
  declareProperty(Prop::DETECTOR_RESOLUTION, EMPTY_DBL(), mandatoryDouble, "Detector pixel resolution, in meters.");
  declareProperty(Prop::CHOPPER_SPEED, EMPTY_DBL(), mandatoryDouble, "Chopper speed, in rpm.");
  declareProperty(Prop::CHOPPER_OPENING, EMPTY_DBL(), mandatoryDouble, "The opening angle between the two choppers, in degrees.");
  declareProperty(Prop::CHOPPER_PAIR_DIST, EMPTY_DBL(), mandatoryDouble, "The gap between two choppers, in meters.");
  declareProperty(Prop::SLIT1_NAME, "", mandatoryString, "Name of the first slit component.");
  declareProperty(Prop::SLIT1_SIZE_LOG, "", mandatoryString, "The sample log entry for the first slit opening.");
  declareProperty(Prop::SLIT2_NAME, "", mandatoryString, "Name of the second slit component.");
  declareProperty(Prop::SLIT2_SIZE_LOG, "", mandatoryString, "The sample log entry for the second slit opening.");
  declareProperty(Prop::TOF_CHANNEL_WIDTH, EMPTY_DBL(), mandatoryDouble, "TOF bin width, in microseconds.");
}

/** Execute the algorithm.
 */
void ReflectometryQResolution::exec() {
  using namespace boost::math;
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);
  API::MatrixWorkspace_sptr reflectedWS = getProperty(Prop::REFLECTED_BEAM_WS);
  API::MatrixWorkspace_sptr directWS = getProperty(Prop::DIRECT_BEAM_WS);
  auto setup = experimentSetup(*reflectedWS);
  API::MatrixWorkspace_sptr outWS;
  outWS = inWS->clone();
  convertToMomentumTransfer(outWS);
  const auto beamFWHM = beamRMSVariation(reflectedWS, setup);
  const auto incidentFWHM = incidentAngularSpread(setup);
  const auto slit1FWHM = slit1AngularSpread(setup);
  const auto &spectrumInfo = inWS->spectrumInfo();
  for (size_t wsIndex = 0; wsIndex < outWS->getNumberHistograms(); ++wsIndex) {
    auto dx = Kernel::make_cow<HistogramData::HistogramDx>(outWS->y(0).size(), 0);
    const auto &wavelengths = inWS->x(wsIndex);
    const auto &qs = outWS->x(wsIndex);
    outWS->setSharedDx(wsIndex, dx);
    if (spectrumInfo.isMonitor(wsIndex) || spectrumInfo.isMasked(wsIndex)) {
      // Skip monitors & masked spectra, leave DX to zero.
      continue;
    }
    auto &resolutions = outWS->mutableDx(wsIndex);
    for (size_t i = 0; i < wavelengths.size(); ++i) {
      const auto wavelength = wavelengths[i] * 1e-10;
      const auto deltaLambda = wavelengthResolution(*inWS, wsIndex, setup, wavelength);
      const auto deltaThetaSq = angularResolutionSquared(inWS, *directWS, wsIndex, setup, beamFWHM, incidentFWHM, slit1FWHM);
      resolutions[i] = qs[i] * std::sqrt(pow<2>(deltaLambda) + deltaThetaSq);
    }
  }
  setProperty(Prop::OUTPUT_WS, outWS);
}

double ReflectometryQResolution::angularResolutionSquared(API::MatrixWorkspace_sptr &ws, const API::MatrixWorkspace &directWS, const size_t wsIndex, const Setup &setup, const double beamFWHM, const double incidentFWHM, const double slit1FWHM) {
  using namespace boost::math;
  const auto waviness = sampleWaviness(ws, directWS, wsIndex, setup, beamFWHM, incidentFWHM);
  const auto slit2FWHM = slit2AngularSpread(*ws, wsIndex, setup);
  const auto &spectrumInfo = ws->spectrumInfo();
  const auto l2 = spectrumInfo.l2(wsIndex);
  const auto braggAngle = 0.5 * spectrumInfo.twoTheta(wsIndex);
  if (setup.sumType == SumType::Q) {
    if (waviness > 0) {
      if (slit1FWHM >= 2. * waviness) {
        return (pow<2>(setup.detectorResolution / l2) + pow<2>(slit2FWHM) + pow<2>(waviness)) / pow<2>(braggAngle);
      } else {
        return (pow<2>(setup.detectorResolution / 2. /l2) + pow<2>(slit2FWHM) + pow<2>(slit1FWHM)) / pow<2>(braggAngle);
      }
    } else {
      if (slit1FWHM > setup.detectorResolution / l2) {
        return (pow<2>(setup.detectorResolution / l2) + pow<2>(slit2FWHM)) / pow<2>(braggAngle);
      } else {
        const double incidentSpread = incidentFWHM;
        return (pow<2>(incidentSpread) + pow<2>(setup.detectorResolution / l2)) / pow<2>(braggAngle);
      }
    }
  } else {  // SumType::LAMBDA
    const auto foregroundWidthLimited = pow<2>(0.68) * (pow<2>(static_cast<double>(setup.foregroundEndPixel - setup.foregroundStartPixel + 1) * setup.pixelSize) + pow<2>(setup.slit2Size / l2)) / pow<2>(braggAngle);
    double angularResolution;
    if (setup.polarized) {
      angularResolution = pow<2>(incidentFWHM / braggAngle);
    } else {
      angularResolution = (pow<2>(incidentFWHM) + pow<2>(waviness)) / pow<2>(braggAngle);
    }
    return std::min(angularResolution, foregroundWidthLimited);
  }
}

double ReflectometryQResolution::beamRMSVariation(API::MatrixWorkspace_sptr &ws, const Setup &setup) {
  // det_fwhm and detdb_fwhm in COSMOS
  using namespace boost::math;
  auto integrate = createChildAlgorithm("Integration");
  integrate->setProperty("InputWorkspace", ws);
  integrate->setProperty("OutputWorkspace", "unused_for_child");
  integrate->setProperty("StartWorkspaceIndex", static_cast<int>(setup.foregroundStartPixel));
  integrate->setProperty("EndWorkspaceIndex", static_cast<int>(setup.foregroundEndPixel));
  integrate->execute();
  API::MatrixWorkspace_const_sptr integratedWS = integrate->getProperty("OutputWorkspace");
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
    variance += thetaDistribution[i] * pow<2>(massCenter - static_cast<double>(i));
  }
  variance /= sum;
  const double pixelSize = getProperty(Prop::PIXEL_SIZE);
  return 2. * std::sqrt(2. * std::log(2.)) * pixelSize * std::sqrt(variance);
}

void ReflectometryQResolution::convertToMomentumTransfer(API::MatrixWorkspace_sptr &ws) {
  auto convert = createChildAlgorithm("ConvertUnits");
  convert->setProperty("InputWorkspace", ws);
  convert->setProperty("OutputWorkspace", "unused_for_child");
  convert->setProperty("Target", "MomentumTransfer");
  convert->execute();
  ws = convert->getProperty("OutputWorkspace");
}

// TODO Find out what on the earth DA means (from COSMOS).
double ReflectometryQResolution::detectorDA(const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup, const double incidentFWHM) {
  // da_det in COSMOS
  using namespace boost::math;
  const auto slitSizeRatio = setup.slit2Size / setup.slit1Size;
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto slit2Detector = setup.slit2SampleDistance + spectrumInfo.l2(wsIndex);
  const auto virtualSourceDist = slit2Detector + (slitSizeRatio * setup.slit1Slit2Distance) / (1. + slitSizeRatio);
  return std::sqrt(pow<2>(incidentFWHM * virtualSourceDist) + pow<2>(setup.detectorResolution));
}

const ReflectometryQResolution::Setup ReflectometryQResolution::experimentSetup(const API::MatrixWorkspace &ws) {
  Setup s;
  s.chopperOpening = inRad(getProperty(Prop::CHOPPER_OPENING));
  s.chopperPairDistance = getProperty(Prop::CHOPPER_PAIR_DIST);
  s.chopperPeriod = 1. / (static_cast<double>(getProperty(Prop::CHOPPER_SPEED)) / 60.);
  s.detectorResolution = getProperty(Prop::DETECTOR_RESOLUTION);
  const std::vector<int> foreground = getProperty(Prop::FOREGROUND);
  const auto lowPixel = static_cast<size_t>(foreground.front());
  const auto highPixel = static_cast<size_t>(foreground.back());
  s.foregroundStartPixel = std::min(lowPixel, highPixel);
  s.foregroundEndPixel = std::max(lowPixel, highPixel);
  s.pixelSize = getProperty(Prop::PIXEL_SIZE);
  s.polarized = getProperty(Prop::POLARIZED);
  s.slit1Slit2Distance = interslitDistance(ws);
  const std::string slit1SizeEntry = getProperty(Prop::SLIT1_SIZE_LOG);
  s.slit1Size = slitSize(ws, slit1SizeEntry);
  const std::string slit2Name = getProperty(Prop::SLIT2_NAME);
  auto instrument = ws.getInstrument();
  auto slit2 = instrument->getComponentByName(slit2Name);
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto samplePos = spectrumInfo.samplePosition();
  s.slit2SampleDistance = (slit2->getPos() - samplePos).norm();
  const std::string slit2SizeEntry = getProperty(Prop::SLIT2_SIZE_LOG);
  s.slit2Size = slitSize(ws, slit2SizeEntry);
  const std::string sumType = getProperty(Prop::SUM_TYPE);
  s.sumType = sumType == SumTypeChoice::LAMBDA ? SumType::LAMBDA : SumType::Q;
  s.tofChannelWidth = getProperty(Prop::TOF_CHANNEL_WIDTH);
  return s;
}

double ReflectometryQResolution::incidentAngularSpread(const Setup &setup) {
  // da in COSMOS
  using namespace boost::math;
  return 0.68 * std::sqrt((pow<2>(setup.slit1Size) + pow<2>(setup.slit2Size))) / setup.slit1Slit2Distance;
}

double ReflectometryQResolution::interslitDistance(const API::MatrixWorkspace &ws) {
  const std::string slit1Name = getProperty(Prop::SLIT1_NAME);
  const std::string slit2Name = getProperty(Prop::SLIT2_NAME);
  auto instrument = ws.getInstrument();
  auto slit1 = instrument->getComponentByName(slit1Name);
  auto slit2 = instrument->getComponentByName(slit2Name);
  return (slit1->getPos() - slit2->getPos()).norm();
}

double ReflectometryQResolution::sampleWaviness(API::MatrixWorkspace_sptr &ws, const API::MatrixWorkspace &directWS, const size_t wsIndex, const Setup &setup, const double beamFWHM, const double incidentFWHM) {
  // om_fwhm in COSMOS
  using namespace boost::math;
  const double daDet = detectorDA(*ws, wsIndex, setup, incidentFWHM);
  // Do only the differing slit sizes branch from COSMOS.
  if (beamFWHM - daDet >= 0) {
    const auto a = std::sqrt(pow<2>(beamFWHM) - pow<2>(daDet));
    if (a >= setup.pixelSize) {
      const auto directL2 = directWS.spectrumInfo().l2(wsIndex);
      return 0.5 * a / directL2;
    } else {
      return 0.;
    }
  }
  return 0.;
}

double ReflectometryQResolution::slit1AngularSpread(const Setup& setup) {
  // S2_fwhm in COSMOS
  return 0.68 * setup.slit1Size / setup.slit1Slit2Distance;
}

double ReflectometryQResolution::slit2AngularSpread(const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup) {
  // s3_fwhm in COSMOS.
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto slit2Detector = setup.slit2SampleDistance + spectrumInfo.l2(wsIndex);
  return 0.68 * setup.slit2Size / slit2Detector;
}

double ReflectometryQResolution::slitSize(const API::MatrixWorkspace &ws, const std::string &logEntry) {
  auto &run = ws.run();
  const double opening = run.getPropertyValueAsType<double>(logEntry);
  const auto &units = run.getProperty(logEntry)->units();
  if (units.empty()) {
    m_log.warning() << "Slit opening entry " << logEntry << " has no unit. Assuming meters.\n";
    return opening;
  } else if (units == "m") {
    return opening;
  } else if (units == "mm") {
    return opening * 1e-3;
  } else {
    m_log.warning() << "Slit opening entry " << logEntry << " has an unknown unit. Assuming meters.\n";
    return opening;
  }
}

double ReflectometryQResolution::wavelengthResolution(const API::MatrixWorkspace &ws, const size_t wsIndex, const Setup &setup, const double wavelength) {
  // err_res in COSMOS
  using namespace boost::math;
  using namespace PhysicalConstants;
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto flightDistance = spectrumInfo.l1() + spectrumInfo.l2(wsIndex);
  const auto chopperResolution = (setup.chopperPairDistance + h * setup.chopperOpening * setup.chopperPeriod / (2. * M_PI * NeutronMass * wavelength)) / (2. * flightDistance);
  const auto detectorResolution = h * setup.tofChannelWidth / (NeutronMass * wavelength * flightDistance);
  // Shouldn't the factor be 0.49?
  return 0.98 * (3. * pow<2>(chopperResolution) + pow<2>(detectorResolution) + 3. * chopperResolution * detectorResolution) / (2. * chopperResolution + detectorResolution);
}

} // namespace Algorithms
} // namespace Mantid

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryBeamStatistics.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include <boost/math/special_functions/pow.hpp>
#include <optional>

namespace {
namespace Prop {
const std::string DETECTOR_RESOLUTION{"DetectorResolution"};
const std::string DIRECT_FOREGROUND{"DirectForeground"};
const std::string DIRECT_WS{"DirectLineWorkspace"};
const std::string FIRST_SLIT_NAME{"FirstSlitName"};
const std::string FIRST_SLIT_SIZE_LOG{"FirstSlitSizeSampleLog"};
const std::string PIXEL_SIZE{"PixelSize"};
const std::string REFLECTED_FOREGROUND{"ReflectedForeground"};
const std::string REFLECTED_WS{"ReflectedBeamWorkspace"};
const std::string SECOND_SLIT_NAME{"SecondSlitName"};
const std::string SECOND_SLIT_SIZE_LOG{"SecondSlitSizeSampleLog"};
} // namespace Prop
/// A conversion factor from e.g. slit opening to FWHM of Gaussian equivalent.
constexpr double FWHM_GAUSSIAN_EQUIVALENT{0.68};
} // namespace

namespace Mantid::Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryBeamStatistics)

const std::string ReflectometryBeamStatistics::LogEntry::BEAM_RMS_VARIATION("beam_stats.beam_rms_variation");
const std::string ReflectometryBeamStatistics::LogEntry::BENT_SAMPLE("beam_stats.bent_sample");
const std::string
    ReflectometryBeamStatistics::LogEntry::FIRST_SLIT_ANGULAR_SPREAD("beam_stats.first_slit_angular_spread");
const std::string ReflectometryBeamStatistics::LogEntry::INCIDENT_ANGULAR_SPREAD("beam_stats.incident_angular_spread");
const std::string ReflectometryBeamStatistics::LogEntry::SAMPLE_WAVINESS("beam_stats.sample_waviness");
const std::string
    ReflectometryBeamStatistics::LogEntry::SECOND_SLIT_ANGULAR_SPREAD("beam_stats.second_slit_angular_spread");

/** Give the gap between the two slits, in meters.
 *
 * @param instrument an instrument which containts the slit components
 * @param slit1Name name of the first slit component
 * @param slit2Name name of the second slit component
 * @return the slit gap, in meters
 */
double ReflectometryBeamStatistics::slitSeparation(const Geometry::Instrument_const_sptr &instrument,
                                                   const std::string &slit1Name, const std::string &slit2Name) {
  auto slit1 = instrument->getComponentByName(slit1Name);
  auto slit2 = instrument->getComponentByName(slit2Name);
  return (slit1->getPos() - slit2->getPos()).norm();
}

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometryBeamStatistics::name() const { return "ReflectometryBeamStatistics"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryBeamStatistics::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryBeamStatistics::category() const { return "ILL\\Reflectometry;Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryBeamStatistics::summary() const {
  return "Calculates statistical quantities of a reflectometry workspace.";
}

/// Return a vector of related algorithms.
const std::vector<std::string> ReflectometryBeamStatistics::seeAlso() const {
  return {"ReflectometryMomentumTransfer", "ReflectometrySumInQ"};
}

/** Initialize the algorithm's properties.
 */
void ReflectometryBeamStatistics::init() {
  auto threeElementArray = std::make_shared<Kernel::ArrayLengthValidator<int>>(3);
  auto mandatoryDouble = std::make_shared<Kernel::MandatoryValidator<double>>();
  auto mandatoryNonnegativeInt = std::make_shared<Kernel::CompositeValidator>();
  mandatoryNonnegativeInt->add<Kernel::MandatoryValidator<int>>();
  auto nonnegativeInt = std::make_shared<Kernel::BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  mandatoryNonnegativeInt->add(nonnegativeInt);
  auto mandatoryString = std::make_shared<Kernel::MandatoryValidator<std::string>>();
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::REFLECTED_WS, "", Kernel::Direction::InOut),
      "A reflected beam workspace.");
  declareProperty(Prop::REFLECTED_FOREGROUND, std::vector<int>(), threeElementArray,
                  "A list of three workspace indices [start, beam centre, end] "
                  "defining the reflected foreground.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::DIRECT_WS, "", Kernel::Direction::InOut),
      "A direct beam workspace.");
  declareProperty(Prop::DIRECT_FOREGROUND, std::vector<int>(), threeElementArray,
                  "A list of three workspace indices [start, beam centre, end] "
                  "defining the direct foreground.");
  declareProperty(Prop::PIXEL_SIZE, EMPTY_DBL(), mandatoryDouble, "Detector pixel size, in meters.");
  declareProperty(Prop::DETECTOR_RESOLUTION, EMPTY_DBL(), mandatoryDouble, "Detector pixel resolution, in meters.");
  declareProperty(Prop::FIRST_SLIT_NAME, "", mandatoryString, "Name of the first slit component.");
  declareProperty(Prop::FIRST_SLIT_SIZE_LOG, "", mandatoryString, "The sample log entry for the first slit opening.");
  declareProperty(Prop::SECOND_SLIT_NAME, "", mandatoryString, "Name of the second slit component.");
  declareProperty(Prop::SECOND_SLIT_SIZE_LOG, "", mandatoryString, "The sample log entry for the second slit opening.");
}

/// Return issues found in input properties.
std::map<std::string, std::string> ReflectometryBeamStatistics::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr reflectedWS = getProperty(Prop::REFLECTED_WS);
  const std::string slit1Name = getProperty(Prop::FIRST_SLIT_NAME);
  auto instrument = reflectedWS->getInstrument();
  auto slit = instrument->getComponentByName(slit1Name);
  if (!slit) {
    issues[Prop::FIRST_SLIT_NAME] = "No component called '" + slit1Name + "' found in " + Prop::REFLECTED_WS;
  }
  const std::string slit2Name = getProperty(Prop::SECOND_SLIT_NAME);
  slit = instrument->getComponentByName(slit2Name);
  if (!slit) {
    issues[Prop::SECOND_SLIT_NAME] = "No component called '" + slit2Name + "' found in " + Prop::REFLECTED_WS;
  }
  return issues;
}

/** Execute the algorithm.
 */
void ReflectometryBeamStatistics::exec() {
  API::MatrixWorkspace_sptr reflectedWS = getProperty(Prop::REFLECTED_WS);
  API::MatrixWorkspace_sptr directWS = getProperty(Prop::DIRECT_WS);
  const auto setup = createSetup(*reflectedWS, *directWS);
  const auto beamFWHM = beamRMSVariation(reflectedWS, setup.foregroundStart, setup.foregroundEnd);
  rmsVariationToLogs(*reflectedWS, beamFWHM);
  const auto directBeamFWHM = beamRMSVariation(directWS, setup.directForegroundStart, setup.directForegroundEnd);
  rmsVariationToLogs(*directWS, directBeamFWHM);
  Statistics statistics;
  statistics.incidentAngularSpread = incidentAngularSpread(setup);
  statistics.sampleWaviness = sampleWaviness(setup, beamFWHM, directBeamFWHM, statistics.incidentAngularSpread);
  statistics.firstSlitAngularSpread = firstSlitAngularSpread(setup);
  statistics.secondSlitAngularSpread = secondSlitAngularSpread(setup);
  statistics.bentSample = bentSample(setup, statistics.sampleWaviness, statistics.firstSlitAngularSpread);
  statisticsToLogs(*reflectedWS, statistics);
}

/** Calculate the beam FWHM or read its value from the sample logs.
 *
 * @param ws a reference workspace
 * @param start foreground start workspace index
 * @param end foreground end workspace index
 * @return FWHM in units of pixel size
 */
double ReflectometryBeamStatistics::beamRMSVariation(API::MatrixWorkspace_sptr &ws, const size_t start,
                                                     const size_t end) {
  // det_fwhm and detdb_fwhm in COSMOS
  std::optional<double> rmsVariation;
  if (ws->run().hasProperty(LogEntry::BEAM_RMS_VARIATION)) {
    try {
      rmsVariation = ws->run().getPropertyValueAsType<double>(LogEntry::BEAM_RMS_VARIATION);
    } catch (std::invalid_argument &) {
      m_log.warning() << "Cannot convert '" + LogEntry::BEAM_RMS_VARIATION +
                             "' sample log into a number. Recalculating the value.\n";
    }
  }
  if (!rmsVariation) {
    using namespace boost::math;
    auto integrate = createChildAlgorithm("Integration");
    integrate->setProperty("InputWorkspace", ws);
    integrate->setProperty("OutputWorkspace", "unused_for_child");
    integrate->setProperty("StartWorkspaceIndex", static_cast<int>(start));
    integrate->setProperty("EndWorkspaceIndex", static_cast<int>(end));
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
    rmsVariation = 2. * std::sqrt(2. * std::log(2.)) * pixelSize * std::sqrt(variance);
  }
  return *rmsVariation;
}

/** Return true if the sample is considered as bent or beam is divergent.
 *
 * @param setup a setup object
 * @param sampleWaviness the value of sample waviness
 * @param firstSlitAngularSpread the value of the RMS angular spread of the
 * first slit
 * @return true if sample is considered bent
 */
bool ReflectometryBeamStatistics::bentSample(const Setup &setup, const double sampleWaviness,
                                             const double firstSlitAngularSpread) {
  return sampleWaviness > 0 && setup.detectorResolution / setup.l2 > firstSlitAngularSpread;
}

/** Generate a setup for the reflected beam experiment.
 *
 * @param ws the reflectivity workspace
 * @param directWS corresponding direct beam workspace
 * @return a setup object
 */
const ReflectometryBeamStatistics::Setup
ReflectometryBeamStatistics::createSetup(const API::MatrixWorkspace &ws, const API::MatrixWorkspace &directWS) {
  Setup s;
  s.detectorResolution = getProperty(Prop::DETECTOR_RESOLUTION);
  const std::vector<int> reflectedForeground = getProperty(Prop::REFLECTED_FOREGROUND);
  auto lowPixel = static_cast<size_t>(reflectedForeground.front());
  auto highPixel = static_cast<size_t>(reflectedForeground.back());
  s.foregroundStart = std::min(lowPixel, highPixel);
  s.foregroundEnd = std::max(lowPixel, highPixel);
  const std::vector<int> directForeground = getProperty(Prop::DIRECT_FOREGROUND);
  lowPixel = static_cast<size_t>(directForeground.front());
  highPixel = static_cast<size_t>(directForeground.back());
  s.directForegroundStart = std::min(lowPixel, highPixel);
  s.directForegroundEnd = std::max(lowPixel, highPixel);
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto reflectedBeamCentre = static_cast<size_t>(reflectedForeground[1]);
  s.l2 = spectrumInfo.l2(reflectedBeamCentre);
  const auto &directSpectrumInfo = directWS.spectrumInfo();
  const auto directBeamCentre = static_cast<size_t>(directForeground[1]);
  s.directL2 = directSpectrumInfo.l2(static_cast<size_t>(directBeamCentre));
  s.pixelSize = getProperty(Prop::PIXEL_SIZE);
  s.slit1Slit2Distance = interslitDistance(ws);
  const std::string slit1SizeEntry = getProperty(Prop::FIRST_SLIT_SIZE_LOG);
  s.slit1Size = slitSize(ws, slit1SizeEntry);
  s.slit1SizeDirectBeam = slitSize(directWS, slit1SizeEntry);
  const std::string slit2Name = getProperty(Prop::SECOND_SLIT_NAME);
  auto instrument = ws.getInstrument();
  auto slit2 = instrument->getComponentByName(slit2Name);
  const auto samplePos = spectrumInfo.samplePosition();
  s.slit2SampleDistance = (slit2->getPos() - samplePos).norm();
  const std::string slit2SizeEntry = getProperty(Prop::SECOND_SLIT_SIZE_LOG);
  s.slit2Size = slitSize(ws, slit2SizeEntry);
  s.slit2SizeDirectBeam = slitSize(directWS, slit2SizeEntry);
  return s;
}

/** Calculate the detector angular resolution.
 *
 * @param setup reflected beam setup
 * @param incidentFWHM spread of the incident beam
 * @return the angular resolution
 */
double ReflectometryBeamStatistics::detectorAngularResolution(const Setup &setup, const double incidentFWHM) {
  // da_det in COSMOS
  using namespace boost::math;
  const auto slitSizeRatio = setup.slit2Size / setup.slit1Size;
  const auto slit2Detector = setup.slit2SampleDistance + setup.l2;
  const auto virtualSourceDist = slit2Detector + (slitSizeRatio * setup.slit1Slit2Distance) / (1. + slitSizeRatio);
  return std::sqrt(pow<2>(incidentFWHM * virtualSourceDist) + pow<2>(setup.detectorResolution));
}

/** Calculate the angular spread due to the first slit.
 *
 * @param setup a reflected beam setup object
 * @return the spread
 */
double ReflectometryBeamStatistics::firstSlitAngularSpread(const Setup &setup) {
  // S2_fwhm in COSMOS
  return FWHM_GAUSSIAN_EQUIVALENT * setup.slit1Size / setup.slit1Slit2Distance;
}

/** Calculate the range of angles in the reflection plane determined by
 *  the collimation.
 *
 * @param setup a setup object
 * @return the incident FWHM
 */
double ReflectometryBeamStatistics::incidentAngularSpread(const Setup &setup) {
  // da in COSMOS
  using namespace boost::math;
  return FWHM_GAUSSIAN_EQUIVALENT * std::sqrt((pow<2>(setup.slit1Size) + pow<2>(setup.slit2Size))) /
         setup.slit1Slit2Distance;
}

/** Give the gap between the two slits, in meters.
 *
 * @param ws the reflectivity workspace
 * @return the slit gap, in meters
 */
double ReflectometryBeamStatistics::interslitDistance(const API::MatrixWorkspace &ws) {
  const std::string slit1Name = getProperty(Prop::FIRST_SLIT_NAME);
  const std::string slit2Name = getProperty(Prop::SECOND_SLIT_NAME);
  auto instrument = ws.getInstrument();
  return slitSeparation(instrument, slit1Name, slit2Name);
}

/**
 * Adds beam RMS variation to sample logs if it doesn't exist yet.
 * @param ws a workspace sample logs of which to change
 * @param variation beam RMS variation value
 */
void ReflectometryBeamStatistics::rmsVariationToLogs(API::MatrixWorkspace &ws, const double variation) {
  auto &run = ws.mutableRun();
  if (!run.hasProperty(LogEntry::BEAM_RMS_VARIATION)) {
    const std::string metres{"m"};
    run.addProperty(LogEntry::BEAM_RMS_VARIATION, variation, metres);
  }
}

/** Calculate sample RMS waviness.
 *
 * @param setup a setup object
 * @param beamFWHM reflected beam RMS variation
 * @param directBeamFWHM direct beam RMS variation
 * @param incidentFWHM incident beam angular spread
 * @return the waviness
 */
double ReflectometryBeamStatistics::sampleWaviness(const Setup &setup, const double beamFWHM,
                                                   const double directBeamFWHM, const double incidentFWHM) {
  // om_fwhm in COSMOS
  using namespace boost::math;
  const double slitSizeTolerance{0.00004}; // From COSMOS.
  if (std::abs(setup.slit1Size - setup.slit1SizeDirectBeam) >= slitSizeTolerance ||
      std::abs(setup.slit2Size - setup.slit2SizeDirectBeam) >= slitSizeTolerance) {
    // Differing slit sizes branch from COSMOS.
    const double daDet = detectorAngularResolution(setup, incidentFWHM);
    if (beamFWHM >= daDet) {
      const auto a = std::sqrt(pow<2>(beamFWHM) - pow<2>(daDet));
      if (a >= setup.pixelSize) {
        return 0.5 * a / setup.directL2;
      }
    }
  } else if (pow<2>(beamFWHM) - pow<2>(directBeamFWHM) >= 0) {
    const auto a = std::sqrt(pow<2>(beamFWHM) - pow<2>(directBeamFWHM));
    if (a >= setup.pixelSize) {
      return 0.5 * a / setup.directL2;
    }
  }
  return 0.;
}

/** Calculate the angular spread due to the second slit.
 *
 * @param setup reflected beam setup
 * @return the spread
 */
double ReflectometryBeamStatistics::secondSlitAngularSpread(const Setup &setup) {
  // s3_fwhm in COSMOS.
  const auto slit2Detector = setup.slit2SampleDistance + setup.l2;
  return FWHM_GAUSSIAN_EQUIVALENT * setup.slit2Size / slit2Detector;
}

/** Read the slit size from sample logs.
 *
 * @param ws workspace to investigate
 * @param logEntry name of the slit opening sample log
 * @return the slit opening, in meters
 */
double ReflectometryBeamStatistics::slitSize(const API::MatrixWorkspace &ws, const std::string &logEntry) {
  auto &run = ws.run();
  const auto opening = run.getPropertyValueAsType<double>(logEntry);
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

/**
 * Add statistics to sample logs overwriting previous values.
 * @param ws a workspace to modify
 * @param statistics statistics to write
 */
void ReflectometryBeamStatistics::statisticsToLogs(API::MatrixWorkspace &ws, const Statistics &statistics) {
  auto &run = ws.mutableRun();
  constexpr bool overwrite{true};
  const std::string radians{"radians"};
  run.addProperty(LogEntry::BENT_SAMPLE, statistics.bentSample ? 1 : 0, overwrite);
  run.addProperty(LogEntry::FIRST_SLIT_ANGULAR_SPREAD, statistics.firstSlitAngularSpread, radians, overwrite);
  run.addProperty(LogEntry::INCIDENT_ANGULAR_SPREAD, statistics.incidentAngularSpread, radians, overwrite);
  run.addProperty(LogEntry::SAMPLE_WAVINESS, statistics.sampleWaviness, radians, overwrite);
  run.addProperty(LogEntry::SECOND_SLIT_ANGULAR_SPREAD, statistics.secondSlitAngularSpread, radians, overwrite);
}

} // namespace Mantid::Reflectometry

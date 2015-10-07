#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid {
namespace MDAlgorithms {

namespace {
// Identifiers for contributions
const char *MODERATOR = "Moderator";
const char *APERTURE = "Aperture";
const char *CHOPPER_ARRIVAL = "Chopper";
const char *CHOPPER_JITTER = "ChopperJitter";
const char *SAMPLE_VOLUME = "SampleVolume";
const char *DETECTOR_DEPTH = "DetectorDepth";
const char *DETECTOR_AREA = "DetectorArea";
const char *DETECTION_TIME = "DetectionTime";
}

/// Returns the number length of the Y vector
unsigned int TobyFitYVector::length() { return 11; }

/**
 *  Construct a Y vector for the current set up.
 */
TobyFitYVector::TobyFitYVector()
    : m_yvector(length(), 0.0), m_curRandNums(NULL), m_randIndex(0),
      m_curObs(NULL), m_curQOmega(NULL), m_moderator(true), m_aperture(true),
      m_chopper(true), m_chopperJitter(true), m_sampleVolume(true),
      m_detectorDepth(true), m_detectorArea(true), m_detectionTime(true) {}

/**
 * Adds the attributes from the vector to the given model
 * @param model :: A reference to the model that will take on the attributes
 */
void TobyFitYVector::addAttributes(TobyFitResolutionModel &model) {
  using API::IFunction;
  model.declareAttribute(MODERATOR, IFunction::Attribute(m_moderator));
  model.declareAttribute(APERTURE, IFunction::Attribute(m_aperture));
  model.declareAttribute(CHOPPER_ARRIVAL, IFunction::Attribute(m_chopper));
  model.declareAttribute(CHOPPER_JITTER, IFunction::Attribute(m_chopperJitter));
  model.declareAttribute(SAMPLE_VOLUME, IFunction::Attribute(m_sampleVolume));
  model.declareAttribute(DETECTOR_DEPTH, IFunction::Attribute(m_detectorDepth));
  model.declareAttribute(DETECTOR_AREA, IFunction::Attribute(m_detectorArea));
  model.declareAttribute(DETECTION_TIME, IFunction::Attribute(m_detectionTime));
}

/**
 * Sets an attribute value and returns a boolean depending on whether it was
 * handled
 * @param name :: The name of the attribute
 * @param value :: Value of the named attribute
 * @returns True if the attribute was handled, false otherwise
 */
void TobyFitYVector::setAttribute(const std::string &name,
                                  const API::IFunction::Attribute &value) {
  // Need to move this type of stuff to IFunction::Attribute. - We should be
  // able to interchange int/bool values
  bool active(true);
  try {
    active = value.asBool();
  } catch (std::runtime_error &) {
    try {
      const int asInt = value.asInt();
      active = (asInt != 0);
    } catch (std::runtime_error &) {
      return;
    }
  }

  if (name == MODERATOR)
    m_moderator = active;
  else if (name == APERTURE)
    m_aperture = active;
  else if (name == CHOPPER_ARRIVAL)
    m_chopper = active;
  else if (name == CHOPPER_JITTER)
    m_chopperJitter = active;
  else if (name == SAMPLE_VOLUME)
    m_sampleVolume = active;
  else if (name == DETECTOR_DEPTH)
    m_detectorDepth = active;
  else if (name == DETECTOR_AREA)
    m_detectorArea = active;
  else if (name == DETECTION_TIME)
    m_detectionTime = active;
  else {
  }
}

/// Returns the length of random numbers required
unsigned int TobyFitYVector::requiredRandomNums() const {
  unsigned int nrand(0);

  if (m_moderator)
    nrand += 1;
  if (m_aperture)
    nrand += 2;
  if (m_chopper)
    nrand += 1;
  if (m_chopperJitter)
    nrand += 1;
  if (m_sampleVolume)
    nrand += 3;
  if (m_detectorDepth)
    nrand += 1;
  if (m_detectorArea)
    nrand += 2;
  if (m_detectionTime)
    nrand += 1;

  return nrand;
}

/**
 * Access a the current vector index in the vector (in order to be able to
 * multiply it with the b matrix)
 * @return The current Y vector
 */
const std::vector<double> &TobyFitYVector::values() const { return m_yvector; }

/**
 * Calculate the values of the integration variables
 * @param randomNums :: A set of random numbers. The size should be atleast the
 * size
 *                      of the number of active attributes
 * @param observation :: The current observation
 * @param qOmega :: The energy change for this point
 * @returns The number of random deviates used
 */
size_t TobyFitYVector::recalculate(const std::vector<double> &randomNums,
                                   const CachedExperimentInfo &observation,
                                   const QOmegaPoint &qOmega) {
  m_curRandNums = &randomNums;
  m_randIndex = 0;
  m_curObs = &observation;
  m_curQOmega = &qOmega;

  calculateModeratorTime();
  calculateAperatureSpread();
  calculateChopperTime();
  calculateSampleContribution();
  calculateDetectorContribution();
  calculateTimeBinContribution();
  size_t randUsed = m_randIndex;

  m_curRandNums = NULL;
  m_randIndex = 0;
  m_curObs = NULL;
  m_curQOmega = NULL;

  return randUsed;
}

//-----------------------------------------------------------------------
// Private members
//-----------------------------------------------------------------------
/**
 * Sample from moderator time distribution
 */
void TobyFitYVector::calculateModeratorTime() {
  m_yvector[TobyFitYVector::ModeratorTime] = 0.0;
  if (m_moderator) {
    const API::ModeratorModel &moderator =
        m_curObs->experimentInfo().moderatorModel();
    m_yvector[TobyFitYVector::ModeratorTime] =
        moderator.sampleTimeDistribution(nextRandomNumber()) * 1e-06;
  }
}

/**
 * Calculate deviation due to finite aperture size
 */
void TobyFitYVector::calculateAperatureSpread() {
  double &apertureWidth =
      m_yvector[TobyFitYVector::ApertureWidthCoord]; // Reference
  double &apertureHeight =
      m_yvector[TobyFitYVector::ApertureHeightCoord]; // Reference
  apertureWidth = 0.0;
  apertureHeight = 0.0;
  if (m_aperture) {
    const std::pair<double, double> &apSize = m_curObs->apertureSize();
    apertureWidth = apSize.first * (nextRandomNumber() - 0.5);
    apertureHeight = apSize.second * (nextRandomNumber() - 0.5);
  }
}

/**
 * Chopper time spread due to the chopper component
 */
void TobyFitYVector::calculateChopperTime() {
  const API::ChopperModel &chopper = m_curObs->experimentInfo().chopperModel(0);
  double &chopTime =
      m_yvector[TobyFitYVector::ChopperTime]; // Note the reference
  chopTime = 0.0;

  if (m_chopper) {
    chopTime = chopper.sampleTimeDistribution(nextRandomNumber());
  }
  if (m_chopperJitter) {
    chopTime += chopper.sampleJitterDistribution(nextRandomNumber());
  }
}

/**
 * Sample over the sample volume
 */
void TobyFitYVector::calculateSampleContribution() {

  if (m_sampleVolume) {
    double &sampleBeamDir = m_yvector[TobyFitYVector::ScatterPointBeam];
    double &samplePerpDir = m_yvector[TobyFitYVector::ScatterPointPerp];
    double &sampleUpDir = m_yvector[TobyFitYVector::ScatterPointUp];

    const Kernel::V3D &boxSize = m_curObs->sampleCuboid();
    sampleBeamDir = boxSize[2] * (nextRandomNumber() - 0.5);
    samplePerpDir = boxSize[0] * (nextRandomNumber() - 0.5);
    sampleUpDir = boxSize[1] * (nextRandomNumber() - 0.5);
  }
}

/**
 * Sample over the detector volume
 */
void TobyFitYVector::calculateDetectorContribution() {
  const Kernel::V3D detectorVolume = m_curObs->detectorVolume();

  double &depth = m_yvector[TobyFitYVector::DetectorDepth];        // reference
  double &width = m_yvector[TobyFitYVector::DetectorWidthCoord];   // reference
  double &height = m_yvector[TobyFitYVector::DetectorHeightCoord]; // reference

  if (m_detectorDepth) {
    depth = detectorVolume[2] * (nextRandomNumber() - 0.5); // Beam
  } else {
    depth = 0.0;
  }

  if (m_detectorArea) {

    width = detectorVolume[0] * (nextRandomNumber() - 0.5);  // Perp
    height = detectorVolume[1] * (nextRandomNumber() - 0.5); // Up
  } else {
    width = 0.0;
    height = 0.0;
  }
}

/**
 * Sample over detector time bin
 */
void TobyFitYVector::calculateTimeBinContribution() {
  double &detectorTime = m_yvector[TobyFitYVector::DetectionTime];
  if (m_detectionTime) {

    const API::ExperimentInfo &exptInfo = m_curObs->experimentInfo();
    const std::pair<double, double> binEdges =
        exptInfo.run().histogramBinBoundaries(m_curQOmega->deltaE);
    const double energyWidth = binEdges.second - binEdges.first;
    const double efixed = m_curObs->getEFixed();
    const double wf = std::sqrt((efixed - m_curQOmega->deltaE) /
                                PhysicalConstants::E_mev_toNeutronWavenumberSq);
    const double factor(3.8323960e-4);
    const double detTimeBin = energyWidth * factor *
                              m_curObs->sampleToDetectorDistance() /
                              std::pow(wf, 3.0);

    detectorTime = detTimeBin * (nextRandomNumber() - 0.5);
  } else
    detectorTime = 0.0;
}

/**
 * Return the next random number and increment the internal used index
 */
const double &TobyFitYVector::nextRandomNumber() {
  return m_curRandNums->at(
      m_randIndex++); // Post-fix increments then returns previous value
}
}
}

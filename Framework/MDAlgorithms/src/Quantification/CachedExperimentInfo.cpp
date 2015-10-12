//
// Includes
//
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid {
namespace MDAlgorithms {
using namespace Geometry;

/**
 * Constructor taking an experiment info & detector id
 * @param exptInfo :: The description of the experiment
 * @param detID :: An ID a detector within the instrument
 */
CachedExperimentInfo::CachedExperimentInfo(const API::ExperimentInfo &exptInfo,
                                           detid_t detID)
    : m_exptInfo(exptInfo), m_efixed(0.0), m_twoTheta(0.0), m_phi(0.0),
      m_modToChop(0.0), m_apertureToChop(0.0), m_chopToSample(0.0),
      m_sampleToDet(0.0), m_beam(Geometry::Z), m_up(Geometry::Y),
      m_horiz(Geometry::X), m_apertureSize(0.0, 0.0), m_sampleWidths(),
      m_detBox(), m_gonimeter(NULL), m_sampleToDetMatrix(0, 0) {
  Instrument_const_sptr instrument = exptInfo.getInstrument();
  initCaches(instrument, detID);
}

/**
 * Returns the efixed value for this detector/experiment
 * @return
 */
double CachedExperimentInfo::getEFixed() const { return m_efixed; }
/**
 * Returns the scattering angle theta in radians
 * @return The scattering angle from the beam direction in radians
 */
double CachedExperimentInfo::twoTheta() const { return m_twoTheta; }

/**
 * Returns the azimuth angle phi in radians
 * @return Phi angle in radians
 */
double CachedExperimentInfo::phi() const { return m_phi; }

/**
 * @return the distance from the moderator to the first chopper in metres.
 * throws if no chopper installed
 */
double CachedExperimentInfo::moderatorToFirstChopperDistance() const {
  return m_modToChop;
}

/**
 * @return the distance from the first aperture to the first chopper in metres.
 * throws if no chopper installed/aperture installed
 */
double CachedExperimentInfo::firstApertureToFirstChopperDistance() const {
  return m_apertureToChop;
}

/**
 *
 * @return the distance from the chopper to the sample in metres
 */
double CachedExperimentInfo::firstChopperToSampleDistance() const {
  return m_chopToSample;
}

/**
 * @return the sample to detector distance in metres
 */
double CachedExperimentInfo::sampleToDetectorDistance() const {
  return m_sampleToDet;
}

/**
 * Returns the aperture dimensions
 */
const std::pair<double, double> &CachedExperimentInfo::apertureSize() const {
  return m_apertureSize;
}

/// @returns the widths of a cube that encloses the sample
const Kernel::V3D &CachedExperimentInfo::sampleCuboid() const {
  return m_sampleWidths;
}

/// Returns a V3D that defines the detector volume.
const Kernel::V3D CachedExperimentInfo::detectorVolume() const {
  const Kernel::V3D &minPoint = m_detBox.minPoint();
  const Kernel::V3D &maxPoint = m_detBox.maxPoint();

  Kernel::V3D volume;
  volume[0] = (maxPoint[0] - minPoint[0]);
  volume[1] = (maxPoint[1] - minPoint[1]);
  volume[2] = (maxPoint[2] - minPoint[2]);
  return volume;
}

/**
 * Computes the matrix required to transform from lab coordinates to detector
 * coordinates
 * @return The rotation matrix required to transform from lab coordinates to
 * detector coordinates
 */
const Kernel::DblMatrix &CachedExperimentInfo::labToDetectorTransform() const {
  return m_gonimeter->getR();
}

/**
 *  @return the matrix required to move from sample coordinates -> detector
 * coordinates
 */
const Kernel::DblMatrix &
CachedExperimentInfo::sampleToDetectorTransform() const {
  return m_sampleToDetMatrix;
}

//-------------------------------------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------------------------------------
/**
 * Cache frequently accessed values
 * @param instrument : The instrument for this run
 * @param detID : The det ID for this observation
 */
void CachedExperimentInfo::initCaches(
    const Geometry::Instrument_const_sptr &instrument, const detid_t detID) {
  // Throws if detector does not exist
  // Takes into account possible detector mapping
  IDetector_const_sptr det = m_exptInfo.getDetectorByID(detID);

  // Instrument distances
  boost::shared_ptr<const ReferenceFrame> refFrame =
      instrument->getReferenceFrame();
  m_beam = refFrame->pointingAlongBeam();
  m_up = refFrame->pointingUp();
  m_horiz = refFrame->pointingHorizontal();

  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
  IComponent_const_sptr aperture =
      instrument->getComponentByName("aperture", 1);
  if (!aperture) {
    throw std::invalid_argument(
        "No component named \"aperture\" found in instrument.");
  }
  IObjComponent_const_sptr firstChopper = instrument->getChopperPoint(0);
  const Kernel::V3D samplePos = sample->getPos();
  const Kernel::V3D beamDir = samplePos - source->getPos();

  // Cache
  m_twoTheta = det->getTwoTheta(samplePos, beamDir);
  m_phi = det->getPhi();
  m_modToChop = firstChopper->getDistance(*source);
  m_apertureToChop = firstChopper->getDistance(*aperture);
  m_chopToSample = sample->getDistance(*firstChopper);
  m_sampleToDet = det->getDistance(*sample);

  // Aperture
  Geometry::BoundingBox apertureBox;
  aperture->getBoundingBox(apertureBox);
  if (apertureBox.isNull()) {
    throw std::invalid_argument("CachedExperimentInfo::initCaches - Aperture "
                                "has no bounding box, cannot sample from it");
  }
  m_apertureSize.first = apertureBox.maxPoint()[0] - apertureBox.minPoint()[0];
  m_apertureSize.second = apertureBox.maxPoint()[1] - apertureBox.minPoint()[1];

  // Sample volume
  const API::Sample &sampleDescription = m_exptInfo.sample();
  const Geometry::Object &shape = sampleDescription.getShape();
  m_sampleWidths = shape.getBoundingBox().width();

  // Detector volume
  // Make sure it encompasses all possible detectors
  det->getBoundingBox(m_detBox);
  if (m_detBox.isNull()) {
    throw std::invalid_argument("CachedExperimentInfo::initCaches - Detector "
                                "has no bounding box, cannot sample from it. "
                                "ID:" +
                                boost::lexical_cast<std::string>(det->getID()));
  }

  const double rad2deg = 180. / M_PI;
  const double thetaInDegs = twoTheta() * rad2deg;
  const double phiInDegs = phi() * rad2deg;

  m_gonimeter = new Goniometer;
  m_gonimeter->makeUniversalGoniometer();
  m_gonimeter->setRotationAngle("phi", thetaInDegs);
  m_gonimeter->setRotationAngle("chi", phiInDegs);
  m_sampleToDetMatrix =
      m_exptInfo.sample().getOrientedLattice().getU() * m_gonimeter->getR();

  // EFixed
  m_efixed = m_exptInfo.getEFixed(det);
}
}
}

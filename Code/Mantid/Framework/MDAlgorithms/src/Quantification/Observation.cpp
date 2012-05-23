//
// Includes
//
#include "MantidMDAlgorithms/Quantification/Observation.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Goniometer.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    using namespace Geometry;

    /**
     * Constructor taking an experiment info & detector id
     * @param exptInfo :: The description of the experiment
     * @param detID :: An ID a detector within the instrument
     */
    Observation::Observation(API::ExperimentInfo_const_sptr exptInfo, detid_t detID)
      : m_exptInfo(exptInfo), m_detID(detID)
    {
      Instrument_const_sptr instrument = exptInfo->getInstrument();
      if(!instrument)
      {
        throw std::runtime_error("Observation::Observation() - No instrument defined for given experiment");
      }

      // Check detector exists, will throw if not
      instrument->getDetector(detID);
    }

    /**
     * Returns the efixed value for this detector/experiment
     * @return
     */
    double Observation::getEFixed() const
    {
      return m_exptInfo->getEFixed(m_detID);
    }

    /**
     * Returns the scattering angle theta in radians
     * @return The scattering angle from the beam direction in radians
     */
    double Observation::twoTheta() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IDetector_const_sptr det = instrument->getDetector(m_detID);
      IObjComponent_const_sptr source = instrument->getSource();
      IObjComponent_const_sptr sample = instrument->getSample();
      const Kernel::V3D samplePos = sample->getPos();
      Kernel::V3D beamDir = samplePos - source->getPos();
      return det->getTwoTheta(samplePos, beamDir);
    }

    /**
     * Returns the azimuth angle phi in radians
     * @return Phi angle in radians
     */
    double Observation::phi() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IDetector_const_sptr det = instrument->getDetector(m_detID);
      return det->getPhi();
    }

    /**
     * @return the distance from the moderator to the first chopper in metres.
     * throws if no chopper installed
     */
    double Observation::moderatorToFirstChopperDistance() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IObjComponent_const_sptr source = instrument->getSource();
      IObjComponent_const_sptr firstChopper = instrument->getChopperPoint(0);
      return firstChopper->getDistance(*source);
    }

    /**
     * @return the distance from the first aperture to the first chopper in metres.
     * throws if no chopper installed/aperture installed
     */
    double Observation::firstApertureToFirstChopperDistance() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IComponent_const_sptr aperture = instrument->getComponentByName("aperture", 1);
      if(!aperture)
      {
        throw std::invalid_argument("No component named \"aperture\" found in instrument.");
      }
      IObjComponent_const_sptr firstChopper = instrument->getChopperPoint(0);
      return firstChopper->getDistance(*aperture);
    }

    /**
     *
     * @return the distance from the chopper to the sample in metres
     */
    double Observation::firstChopperToSampleDistance() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IObjComponent_const_sptr firstChopper = instrument->getChopperPoint(0);
      IObjComponent_const_sptr sample = instrument->getSample();
      return sample->getDistance(*firstChopper);
    }

    /**
     * @return the sample to detector distance in metres
     */
    double Observation::sampleToDetectorDistance() const
    {
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IDetector_const_sptr det = instrument->getDetector(m_detID);
      IObjComponent_const_sptr sample = instrument->getSample();
      return det->getDistance(*sample);
    }

    /**
     * Returns a V3D for a randomly sampled point within the detector volume
     * @param randInBeamDir :: A flat random number
     * @param randInUpDir :: A flat random number
     * @param randInHorizontalDir :: A flat random number
     * @return A random point of detector in the detector volume. The returned vector is oriented with the instrument's
     * reference frame
     */
    const Kernel::V3D Observation::sampleOverDetectorVolume(const double randInBeamDir, const double randInUpDir, const double randInHorizontalDir) const
    {
      // Best thing we can do is use the bounding box
      Instrument_const_sptr instrument = m_exptInfo->getInstrument();
      IDetector_const_sptr det = instrument->getDetector(m_detID);
      Geometry::BoundingBox boundBox;
      det->getBoundingBox(boundBox);
      if(boundBox.isNull())
      {
        std::ostringstream os;
        os << "Observation::sampleOverDetectorVolume - The detector with ID=" << det->getID() << " has no bounding box. Cannot estimate size.";
        throw std::invalid_argument(os.str());
      }

      const Kernel::V3D & minPoint = boundBox.minPoint();
      const Kernel::V3D & maxPoint = boundBox.maxPoint();

      boost::shared_ptr<const Geometry::ReferenceFrame> refFrame = m_exptInfo->getInstrument()->getReferenceFrame();
      Kernel::V3D detectionPoint;

      detectionPoint[refFrame->pointingAlongBeam()] =
          (randInBeamDir - 0.5)*(maxPoint[refFrame->pointingAlongBeam()] - minPoint[refFrame->pointingAlongBeam()]);

      detectionPoint[refFrame->pointingUp()] =
          (randInUpDir - 0.5)*(maxPoint[refFrame->pointingUp()] - minPoint[refFrame->pointingUp()]);

      detectionPoint[refFrame->pointingHorizontal()] =
          (randInHorizontalDir - 0.5)*(maxPoint[refFrame->pointingHorizontal()] - minPoint[refFrame->pointingHorizontal()]);

      return detectionPoint;
    }

    /**
     * Computes the matrix required to transform from lab coordinates to detector coordinates
     * @return The rotation matrix required to transform from lab coordinates to detector coordinates
     */
    Kernel::DblMatrix Observation::labToDetectorTransform() const
    {
      static const double rad2deg = 180./M_PI;
      const double thetaInDegs = twoTheta()*rad2deg;
      const double phiInDegs = phi()*rad2deg;

      Goniometer detGoniometer;
      detGoniometer.makeUniversalGoniometer();
      detGoniometer.setRotationAngle("phi", thetaInDegs);
      detGoniometer.setRotationAngle("chi", phiInDegs);
      return detGoniometer.getR();
    }

  }
}

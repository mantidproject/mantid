#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    /// Define the static member
    const char * TobyFitYVector::IDENTIFIERS[NUM_OF_VARS] = {
        "ModeratorDepartureTime",
        "Aperture",
        "Aperture",
        "ChopperArrivalTime",
        "SampleVolume",
        "SampleVolume",
        "SampleVolume",
        "DetectorDepth",
        "DetectorArea",
        "DetectorArea",
        "DetectionTime",
    };

    /// Returns the number of parameters
    unsigned int TobyFitYVector::variableCount()
    {
      return NUM_OF_VARS;
    }

    /**
     *  Return a string identifier for the given attribute
     * @param An enumerated variable. Note that two variables can have the same
     * identifier if each forms part of an attribute,
     * e.g. 1 attribute SampleVolume maps to 3 variables: {X,Y,Z}
     */
    const char * TobyFitYVector::identifier(const unsigned int variable)
    {
      assert(variable < NUM_OF_VARS);
      return IDENTIFIERS[variable];
    }

    /**
     *  Construct a Y vector for the current set up
     * @param tfResModel :: A reference to the current TobyFit model object to check
     * which parameters are active in this run
     */
    TobyFitYVector::TobyFitYVector(const TobyFitResolutionModel & tfResModel)
    : m_tfResModel(tfResModel), m_yvector(variableCount(), 0.0),
      m_curRandNums(NULL), m_curObs(NULL), m_curQOmega(NULL)
    {
    }

    /**
     * Access a the current vector index in the vector (in order to be able to multiply it with the b matrix)
     * @return The current Y vector
     */
    const std::vector<double> & TobyFitYVector::values() const
    {
      return m_yvector;
    }

    /**
     * Calculate the values of the integration variables
     * @param randomNums :: A set of at least variableCount() random numbers
     * @param observation :: The current observation
     * @param qOmega :: The energy change for this point
     * @returns The number of random deviates used
     */
    size_t TobyFitYVector::recalculate(const std::vector<double> & randomNums,
        const Observation & observation,
        const QOmegaPoint & qOmega)
    {
      m_curRandNums = &randomNums;
      m_curObs = &observation;
      m_curQOmega = &qOmega;

      calculateModeratorTime();
      calculateAperatureSpread();
      calculateChopperTime();
      calculateSampleContribution();
      calculateDetectorContribution();
      calculateTimeBinContribution();

      m_curRandNums = NULL;
      m_curObs = NULL;
      m_curQOmega = NULL;

      return variableCount();
    }

    //-----------------------------------------------------------------------
    // Private members
    //-----------------------------------------------------------------------
    /**
     * Sample from moderator time distribution
     */
    void TobyFitYVector::calculateModeratorTime()
    {
      const Variable vecPos = TobyFitYVector::ModeratorTime;
      if(setToZeroIfInactive(vecPos)) return;

      const API::ModeratorModel & moderator = m_curObs->experimentInfo()->moderatorModel();
      m_yvector[vecPos] = moderator.sampleTimeDistribution(m_curRandNums->at(vecPos));
    }

    /**
     * Calculate deviation due to finite aperture size
     */
    void TobyFitYVector::calculateAperatureSpread()
    {
      const Variable vecPos1 = TobyFitYVector::ApertureWidthCoord;
      const Variable vecPos2 = TobyFitYVector::ApertureHeightCoord;
      // Inactive if any is inactive
      bool inactive = setToZeroIfInactive(vecPos1);
      inactive |= setToZeroIfInactive(vecPos2);

      if(inactive) return;

      Geometry::Instrument_const_sptr instrument = m_curObs->experimentInfo()->getInstrument();
      Geometry::IComponent_const_sptr aperture = instrument->getComponentByName("aperture");
      if(!aperture)
      {
        throw std::runtime_error("TobyFitYVector::calculateAperatureSpread - Instrument has no defined aperture component!");
      }
      // Rough size
      Geometry::BoundingBox boundBox;
      aperture->getBoundingBox(boundBox);
      const Kernel::V3D & minPoint = boundBox.minPoint();
      const Kernel::V3D & maxPoint = boundBox.maxPoint();
      // Orientation
      boost::shared_ptr<const Geometry::ReferenceFrame> refFrame = instrument->getReferenceFrame();
      Geometry::PointingAlong upDir = refFrame->pointingUp();
      Geometry::PointingAlong horizontalDir = refFrame->pointingHorizontal();
      const double width(maxPoint[horizontalDir] - minPoint[horizontalDir]),
          height(maxPoint[upDir] - minPoint[upDir]);

      m_yvector[vecPos1] = width * (m_curRandNums->at(vecPos1) - 0.5);
      m_yvector[vecPos2] = height * (m_curRandNums->at(vecPos2) - 0.5);
    }

    /**
     * Sample over the sample volume
     */
    void TobyFitYVector::calculateSampleContribution()
    {
      const Variable vecPos1 = TobyFitYVector::ScatterPointX;
      const Variable vecPos2 = TobyFitYVector::ScatterPointY;
      const Variable vecPos3 = TobyFitYVector::ScatterPointZ;

      // Inactive if any is inactive
      bool inactive = setToZeroIfInactive(vecPos1);
      inactive |= setToZeroIfInactive(vecPos2);
      inactive |= setToZeroIfInactive(vecPos3);

      if(inactive) return;

      // Sample volume
      const API::Sample & sampleDescription = m_curObs->experimentInfo()->sample();
      const Geometry::Object & shape = sampleDescription.getShape();
      if(shape.hasValidShape())
      {
        throw std::runtime_error("TobyFitYVector::calculateSampleContribution - Sample has not defined shape");
      }
      const Geometry::BoundingBox & bbox = shape.getBoundingBox();
      const Kernel::V3D boxSize(bbox.width());
      boost::shared_ptr<const Geometry::ReferenceFrame> refFrame = m_curObs->experimentInfo()->getInstrument()->getReferenceFrame();

      m_yvector[vecPos1] = boxSize[refFrame->pointingHorizontal()] * (m_curRandNums->at(vecPos1));
      m_yvector[vecPos2] = boxSize[refFrame->pointingAlongBeam()] * (m_curRandNums->at(vecPos2));
      m_yvector[vecPos2] = boxSize[refFrame->pointingUp()] * (m_curRandNums->at(vecPos3));
    }

    /**
     * Chopper time spread due to the chopper component
     */
    void TobyFitYVector::calculateChopperTime()
    {
      const Variable vecPos = TobyFitYVector::ChopperTime;
      if(setToZeroIfInactive(vecPos)) return;

      const API::ChopperModel & chopper = m_curObs->experimentInfo()->chopperModel(0);
      m_yvector[vecPos] = chopper.sampleTimeDistribution(m_curRandNums->at(vecPos));
    }

    /**
     * Sample over the detector volume
     */
    void TobyFitYVector::calculateDetectorContribution()
    {
      const Kernel::V3D detectionPoint =
          m_curObs->sampleOverDetectorVolume(m_curRandNums->at(TobyFitYVector::DetectorDepth),
              m_curRandNums->at(TobyFitYVector::DetectorHeightCoord),
              m_curRandNums->at(TobyFitYVector::DetectorWidthCoord));

      calculateDetectorDepthContribution(detectionPoint);
      calculateDetectorSpreadContribution(detectionPoint);
    }

    /**
     * Sample over the depth coordinate
     * @param detectionPoint :: A reference to the randomly sampled detection point within the detector
     */
    void TobyFitYVector::calculateDetectorDepthContribution(const Kernel::V3D & detectionPoint)
    {
      const Variable vecPos = TobyFitYVector::DetectorDepth;
      if(setToZeroIfInactive(vecPos)) return;

      API::ExperimentInfo_const_sptr exptInfo = m_curObs->experimentInfo();
      Geometry::Instrument_const_sptr instrument = exptInfo->getInstrument();

      m_yvector[vecPos] = detectionPoint[instrument->getReferenceFrame()->pointingAlongBeam()];
    }

    /**
     * Sample over the detector face area
     * @param detectionPoint :: A reference to the randomly sampled detection point within the detector
     */
    void TobyFitYVector::calculateDetectorSpreadContribution(const Kernel::V3D & detectionPoint)
    {
      const Variable vecPos1 = TobyFitYVector::DetectorHeightCoord;
      const Variable vecPos2 = TobyFitYVector::DetectorWidthCoord;

      // Inactive if any is inactive
      bool inactive = setToZeroIfInactive(vecPos1);
      inactive |= setToZeroIfInactive(vecPos2);
      if(inactive) return;

      API::ExperimentInfo_const_sptr exptInfo = m_curObs->experimentInfo();
      Geometry::Instrument_const_sptr instrument = exptInfo->getInstrument();

      m_yvector[vecPos1] = detectionPoint[instrument->getReferenceFrame()->pointingUp()];
      m_yvector[vecPos2] = detectionPoint[instrument->getReferenceFrame()->pointingHorizontal()];
    }

    /**
     * Sample over detector time bin
     */
    void TobyFitYVector::calculateTimeBinContribution()
    {
      const Variable vecPos = TobyFitYVector::DetectionTime;
      if(setToZeroIfInactive(vecPos)) return;

      API::ExperimentInfo_const_sptr exptInfo = m_curObs->experimentInfo();
      const std::pair<double, double> binEdges = exptInfo->run().histogramBinBoundaries(m_curQOmega->deltaE);
      const double energyWidth = binEdges.second - binEdges.first;
      const double efixed = m_curObs->getEFixed();
      const double kf = std::sqrt((efixed - m_curQOmega->deltaE)*PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double factor(3.8323960e-4);
      const double detTimeBin = energyWidth * factor * m_curObs->sampleToDetectorDistance() / std::pow(kf, 3.0);

      m_yvector[vecPos] = detTimeBin * (m_curRandNums->at(vecPos) - 0.5);
    }

    /**
     * If the variable is in active then set its contribution to zero and return true
     * @param attr :: An enumerated variable
     * @returns True if the variable's contribution has been zeroed
     */
    bool TobyFitYVector::setToZeroIfInactive(const Variable & attr)
    {
      if(m_tfResModel.useAttribute(attr)) return false;

      m_yvector[attr] = 0.0;
      return true;
    }

  }
}

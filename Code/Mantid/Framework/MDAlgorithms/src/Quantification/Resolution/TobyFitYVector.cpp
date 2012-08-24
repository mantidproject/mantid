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
    TobyFitYVector::TobyFitYVector() : m_yvector(variableCount(), 0.0), m_attrStates(variableCount(), true),
      m_curRandNums(NULL), m_curObs(NULL), m_curQOmega(NULL)
    {
    }

    /**
     * Sets an attribute on/off
     * @param name :: The name of the attribute
     * @param active :: 1 if active, 0 if not
     */
    void TobyFitYVector::setAttribute(const std::string & name, const int active)
    {
      for(unsigned int i = 0; i < variableCount(); ++i)
      {
        if(name == identifier(i))
        {
          m_attrStates[i] = active > 0; // Active if greater than zero
        }
      }
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
        const CachedExperimentInfo & observation,
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

      const API::ModeratorModel & moderator = m_curObs->experimentInfo().moderatorModel();
      m_yvector[vecPos] = moderator.sampleTimeDistribution(m_curRandNums->at(vecPos))*1e-06;
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

      const std::pair<double,double> & apSize = m_curObs->apertureSize();

      m_yvector[vecPos1] = apSize.first * (m_curRandNums->at(vecPos1) - 0.5);
      m_yvector[vecPos2] = apSize.second * (m_curRandNums->at(vecPos2) - 0.5);
    }

    /**
     * Chopper time spread due to the chopper component
     */
    void TobyFitYVector::calculateChopperTime()
    {
      const Variable vecPos = TobyFitYVector::ChopperTime;
      if(setToZeroIfInactive(vecPos)) return;

      const API::ChopperModel & chopper = m_curObs->experimentInfo().chopperModel(0);
      double & chopTime = m_yvector[vecPos];
      chopTime = chopper.sampleTimeDistribution(m_curRandNums->at(vecPos));
    }

    /**
     * Sample over the sample volume
     */
    void TobyFitYVector::calculateSampleContribution()
    {
      const Variable vecPos1 = TobyFitYVector::ScatterPointBeam;
      const Variable vecPos2 = TobyFitYVector::ScatterPointPerp;
      const Variable vecPos3 = TobyFitYVector::ScatterPointUp;

      // Inactive if any is inactive
      bool inactive = setToZeroIfInactive(vecPos1);
      inactive |= setToZeroIfInactive(vecPos2);
      inactive |= setToZeroIfInactive(vecPos3);

      if(inactive) return;

      const Kernel::V3D & boxSize = m_curObs->sampleCuboid();
      m_yvector[vecPos1] = boxSize[2]*(m_curRandNums->at(vecPos1) - 0.5);
      m_yvector[vecPos2] = boxSize[0]*(m_curRandNums->at(vecPos2) - 0.5);
      m_yvector[vecPos3] = boxSize[1]*(m_curRandNums->at(vecPos3) - 0.5);
    }

    /**
     * Sample over the detector volume
     */
    void TobyFitYVector::calculateDetectorContribution()
    {
      const Kernel::V3D detectionPoint =
          m_curObs->sampleOverDetectorVolume(m_curRandNums->at(TobyFitYVector::DetectorDepth),
                                             m_curRandNums->at(TobyFitYVector::DetectorWidthCoord),
                                             m_curRandNums->at(TobyFitYVector::DetectorHeightCoord));

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

      m_yvector[vecPos] = 0.5*detectionPoint[2]; // beam
    }

    /**
     * Sample over the detector face area
     * @param detectionPoint :: A reference to the randomly sampled detection point within the detector
     */
    void TobyFitYVector::calculateDetectorSpreadContribution(const Kernel::V3D & detectionPoint)
    {
      const Variable vecPos1 = TobyFitYVector::DetectorWidthCoord;
      const Variable vecPos2 = TobyFitYVector::DetectorHeightCoord;

      // Inactive if any is inactive
      bool inactive = setToZeroIfInactive(vecPos1);
      inactive |= setToZeroIfInactive(vecPos2);
      if(inactive) return;

      m_yvector[vecPos1] = 0.5*detectionPoint[0]; // perp
      m_yvector[vecPos2] = 0.5*detectionPoint[1]; // up
    }

    /**
     * Sample over detector time bin
     */
    void TobyFitYVector::calculateTimeBinContribution()
    {
      const Variable vecPos = TobyFitYVector::DetectionTime;
      if(setToZeroIfInactive(vecPos)) return;

      const API::ExperimentInfo & exptInfo = m_curObs->experimentInfo();
      const std::pair<double, double> binEdges = exptInfo.run().histogramBinBoundaries(m_curQOmega->deltaE);
      const double energyWidth = binEdges.second - binEdges.first;
      const double efixed = m_curObs->getEFixed();
      const double wf = std::sqrt((efixed - m_curQOmega->deltaE)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double factor(3.8323960e-4);
      const double detTimeBin = energyWidth * factor * m_curObs->sampleToDetectorDistance() / std::pow(wf, 3.0);

      m_yvector[vecPos] = detTimeBin * (m_curRandNums->at(vecPos) - 0.5);
    }

    /**
     * If the variable is in active then set its contribution to zero and return true
     * @param attr :: An enumerated variable
     * @returns True if the variable's contribution has been zeroed
     */
    bool TobyFitYVector::setToZeroIfInactive(const Variable & attr)
    {
      if(m_attrStates[attr]) return false;
      m_yvector[attr] = 0.0;
      return true;
    }

  }
}

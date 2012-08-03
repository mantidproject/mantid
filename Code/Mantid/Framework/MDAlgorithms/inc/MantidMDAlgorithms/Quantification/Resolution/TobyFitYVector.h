#ifndef MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_
#define MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

#include <cassert>
#include <vector>

namespace Mantid
{
  namespace MDAlgorithms
  {
    //
    // Forward declarations
    //
    struct QOmegaPoint;
    class CachedExperimentInfo;

    /**
     * Defines a vector of independent integration variables that are transformed using the
     * B matrix to a set of resolution integration variables.
     *
     * Takes a reference to the current resolution model to check which variables are active
     * and also references to the current observation & event point.
     *
     * Defines the parameters for the TobyFit monte carlo resolution model
     *
     * There is an enumeration in order to keep track of the parameter
     * order as this is important in computing the correct matrix
     * elements. name() computes a string name for a parameter.
     *
     */
    class DLLExport TobyFitYVector
    {
    public:
      /// Enumerate the parameters. Do NOT change the parameters or order without an
      /// understanding of what it will do to the resolution calculation. The
      /// enumeration maps to positions in a matrix so changing these
      /// will have serious consequences
      enum Variable {
        ModeratorTime, // deviation in departure time from moderator surface
        ApertureWidthCoord, // width-coordinate of neutron at apperture
        ApertureHeightCoord, // height-coordinate of neutron at apperture
        ChopperTime, // deviation in time of arrival at chopper
        ScatterPointX, // x-coordinate of point of scattering in sample frame
        ScatterPointY, // y-coordinate of point of scattering in sample frame
        ScatterPointZ, // z-coordinate of point of scattering in sample frame
        DetectorDepth, // depth into detector where neutron was detected
        DetectorWidthCoord, // x-coordinate of point of detection in detector frame
        DetectorHeightCoord, // y-coordinate of point of detection in detector frame
        DetectionTime, // deviation in detection time of neutron
      };
      /// Returns the number of parameters
      static unsigned int variableCount();
      /// Return a string identifier for the given attribute
      static const char * identifier(const unsigned int variable);

      /// Construct a Y vector for the current model
      TobyFitYVector();
      /// Set an attribute on/off
      void setAttribute(const std::string & name, const int active);

      /// Access a the current vector index in the vector (in order to be able to multiply it with the b matrix)
      const std::vector<double> & values() const;
      /// Calculate the values of the integration variables for
      /// the given random variates
      size_t recalculate(const std::vector<double> & randomNums,
                       const CachedExperimentInfo & observation,
                       const QOmegaPoint & qOmega);

    private:
      /// Sample from moderator time distribution
      void calculateModeratorTime();
      /// Aperature contribution
      void calculateAperatureSpread();
      /// Chopper time spread
      void calculateChopperTime();
      /// Sample over the sample volume
      void calculateSampleContribution();
      /// Sample over the detector volume
      void calculateDetectorContribution();
      /// Sample over the detector volume
      void calculateDetectorDepthContribution(const Kernel::V3D & detectionPoint);
      /// Sample over the detector volume
      void calculateDetectorSpreadContribution(const Kernel::V3D & detectionPoint);
      /// Sample over detector time bin
      void calculateTimeBinContribution();

      /// If the variable is in active then set its contrib to zero and return true
      bool setToZeroIfInactive(const Variable & attr);

      /// The total number of variables
      enum { NUM_OF_VARS = 11 };
      /// A static list of string identifiers
      static const char * IDENTIFIERS[NUM_OF_VARS];

      /// The values for the current observation
      std::vector<double> m_yvector;
      /// Flags indicating active attributes
      std::vector<bool> m_attrStates;

      /// A pointer to the current set of random numbers
      const std::vector<double> * m_curRandNums;
      /// A pointer to the current observation
      const CachedExperimentInfo *m_curObs;
      /// The current point in Q-DeltaE space
      const QOmegaPoint *m_curQOmega;
    };

  }
}

#endif /* MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_ */

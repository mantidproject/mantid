#ifndef MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_
#define MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/IFunction.h"

#include <cassert>
#include <vector>

namespace Mantid {
namespace MDAlgorithms {
//
// Forward declarations
//
struct QOmegaPoint;
class CachedExperimentInfo;
class TobyFitResolutionModel;

/**
 * Defines a vector of independent integration variables that are transformed
 *using the
 * B matrix to a set of resolution integration variables.
 *
 * Takes a reference to the current resolution model to check which variables
 *are active
 * and also references to the current observation & event point.
 *
 * Defines the parameters for the TobyFit monte carlo resolution model
 *
 * There is an enumeration in order to keep track of the parameter
 * order as this is important in computing the correct matrix
 * elements. name() computes a string name for a parameter.
 *
 */
class DLLExport TobyFitYVector {
public:
  /// Enumerate the parameters. Do NOT change the parameter values without
  /// understanding of what it will do to the resolution calculation. The
  /// enumeration maps to positions in the TobyFitBMatrix matrix so changing
  /// these
  /// will have consequences
  enum Variable {
    ModeratorTime = 0, // deviation in departure time from moderator surface
    ApertureWidthCoord = 1,  // width-coordinate of neutron at apperture
    ApertureHeightCoord = 2, // height-coordinate of neutron at apperture
    ChopperTime = 3,         // deviation in time of arrival at chopper
    ScatterPointBeam =
        4, // beam direction coordinate of point of scattering in sample frame
    ScatterPointPerp =
        5, // perpendicular-coordinate of point of scattering in sample frame
    ScatterPointUp = 6, // up-coordinate of point of scattering in sample frame
    DetectorDepth = 7,  // depth into detector where neutron was detected
    DetectorWidthCoord =
        8, // width-coordinate of point of detection in detector frame
    DetectorHeightCoord =
        9, // height-coordinate of point of detection in detector frame
    DetectionTime = 10 // deviation in detection time of neutron
  };

  /// Returns the number of parameters, i.e. length of the Y vector
  static unsigned int length();

  /// Construct a Y vector for the current model
  TobyFitYVector();
  /// Adds the attributes from the vector to the given model
  void addAttributes(TobyFitResolutionModel &model);
  /// Set an attribute on/off and return whether it was handled here
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &value);
  /// Returns the number of random numbers required for the current number of
  /// active parameters
  unsigned int requiredRandomNums() const;

  /// Access a the current vector index in the vector (in order to be able to
  /// multiply it with the b matrix)
  const std::vector<double> &values() const;
  /// Calculate the values of the integration variables for
  /// the given random variates
  size_t recalculate(const std::vector<double> &randomNums,
                     const CachedExperimentInfo &observation,
                     const QOmegaPoint &qOmega);

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
  /// Sample over detector time bin
  void calculateTimeBinContribution();

  /// Return the next random number
  const double &nextRandomNumber();

  /// The values for the current observation
  std::vector<double> m_yvector;
  /// A pointer to the current set of random numbers
  const std::vector<double> *m_curRandNums;
  // An index in to the random number vector
  size_t m_randIndex;
  /// A pointer to the current observation
  const CachedExperimentInfo *m_curObs;
  /// The current point in Q-DeltaE space
  const QOmegaPoint *m_curQOmega;

  // Flags marking whether attributes are active
  bool m_moderator, m_aperture, m_chopper, m_chopperJitter, m_sampleVolume,
      m_detectorDepth, m_detectorArea, m_detectionTime;
};
}
}

#endif /* MANTID_MDALGORITHMS_TOBYFITYVECTOR_H_ */

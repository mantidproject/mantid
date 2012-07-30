#ifndef MANTID_MDALGORITHMS_OBSERVATION_H_
#define MANTID_MDALGORITHMS_OBSERVATION_H_
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
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
     * Defines information about a neutron event within a given experiment
     * that was detected by a detector with a given ID.
     *
     * It also serves as a cache for storing quicker lookups to frequently
     * used distances and values, i.e twoTheta, phi etc.
     */
    class DLLExport Observation
    {
    public:
      /// Constructor
      Observation(API::ExperimentInfo_const_sptr exptInfo, const detid_t detID);

      /// Return the experiment info
      inline API::ExperimentInfo_const_sptr experimentInfo() const { return m_exptInfo; }
      /// Returns the efixed value for this detector/experiment
      double getEFixed() const;
      /// Returns the scattering angle theta in radians
      double twoTheta() const;
      /// Returns the azimuth angle phi in radians
      double phi() const;

      /// Returns the distance from the moderator to the first chopper in metres
      double moderatorToFirstChopperDistance() const;
      /// Returns the distance from the first aperture to the first chopper in metres
      double firstApertureToFirstChopperDistance() const;
      /// Returns the distance from the chopper to the sample in metres
      double firstChopperToSampleDistance() const;
      /// Returns the sample to detector distance in metres
      double sampleToDetectorDistance() const;

      /// Returns a V3D for a randomly sampled point within the detector volume
      const Kernel::V3D sampleOverDetectorVolume(const double rand1, const double rand2, const double rand3) const;

      /// Returns the D Matrix. Converts from lab coordindates -> detector coordinates
      Kernel::DblMatrix labToDetectorTransform() const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(Observation);
      DISABLE_COPY_AND_ASSIGN(Observation);

      /// A pointer to the experiment description
      API::ExperimentInfo_const_sptr m_exptInfo;
      /// The detector ID
      detid_t m_detID;
    };
  }

}


#endif /* MANTID_MDALGORITHMS_OBSERVATION_H_ */

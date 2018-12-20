#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class IObject;
class SampleEnvironment;
} // namespace Geometry

namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel
namespace Algorithms {
class IBeamProfile;

/**
  Defines a volume where interactions of Tracks and Objects can take place.
  Given an initial Track, end point & wavelengths it calculates the absorption
  correction factor.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_ALGORITHMS_DLL MCInteractionVolume {
public:
  MCInteractionVolume(const API::Sample &sample,
                      const Geometry::BoundingBox &activeRegion,
                      const size_t maxScatterAttempts = 5000);
  // No creation from temporaries as we store a reference to the object in
  // the sample
  MCInteractionVolume(const API::Sample &&sample,
                      const Geometry::BoundingBox &&activeRegion) = delete;

  const Geometry::BoundingBox &getBoundingBox() const;
  double calculateAbsorption(Kernel::PseudoRandomNumberGenerator &rng,
                             const Kernel::V3D &startPos,
                             const Kernel::V3D &endPos, double lambdaBefore,
                             double lambdaAfter) const;

private:
  const boost::shared_ptr<Geometry::IObject> m_sample;
  const Geometry::SampleEnvironment *m_env;
  const Geometry::BoundingBox m_activeRegion;
  const size_t m_maxScatterAttempts;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_ */

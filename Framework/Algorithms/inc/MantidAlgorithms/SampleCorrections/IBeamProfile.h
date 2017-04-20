#ifndef MANTID_ALGORITHMS_IBEAMPROFILE_H_
#define MANTID_ALGORITHMS_IBEAMPROFILE_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Algorithms {

/**

  Base class for all classes defining a beam profile.

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
class MANTID_ALGORITHMS_DLL IBeamProfile {
public:
  struct Ray {
    Kernel::V3D startPos;
    Kernel::V3D unitDir;
  };

  virtual ~IBeamProfile() = default;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const = 0;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                            const Geometry::BoundingBox &) const = 0;
  virtual Geometry::BoundingBox
  defineActiveRegion(const API::Sample &) const = 0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_IBEAMPROFILE_H_ */

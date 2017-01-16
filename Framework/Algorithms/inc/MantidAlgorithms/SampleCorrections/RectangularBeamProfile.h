#ifndef MANTID_ALGORITHMS_RECTANGULARBEAMPROFILE_H_
#define MANTID_ALGORITHMS_RECTANGULARBEAMPROFILE_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidKernel/V3D.h"

#include <array>

namespace Mantid {
namespace Geometry {
class ReferenceFrame;
}
namespace Algorithms {

/**
  Defines a flat, rectangular beam profile that has a width, height and center
  point. The profile is assumed infinitely thin.

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
class MANTID_ALGORITHMS_DLL RectangularBeamProfile final : public IBeamProfile {
public:
  RectangularBeamProfile(const Geometry::ReferenceFrame &frame,
                         const Kernel::V3D &center, double width,
                         double height);

  IBeamProfile::Ray
  generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const override;
  IBeamProfile::Ray
  generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                const Geometry::BoundingBox &bounds) const override;
  Geometry::BoundingBox defineActiveRegion(const API::Sample &) const override;

private:
  const unsigned short m_upIdx;
  const unsigned short m_beamIdx;
  const unsigned short m_horIdx;
  const double m_width;
  const double m_height;
  std::array<double, 3> m_min;
  Kernel::V3D m_beamDir;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RECTANGULARBEAMPROFILE_H_ */

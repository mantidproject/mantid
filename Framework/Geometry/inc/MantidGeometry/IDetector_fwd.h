#ifndef MANTID_GEOMETRY_IDETECTOR_FWD_H_
#define MANTID_GEOMETRY_IDETECTOR_FWD_H_

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
/**
  This file provides forward declarations for Mantid::Geometry::IDetector

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// Forward declare of Mantid::Geometry::IDetector
class IDetector;

/// Shared pointer to an IDetector object
using IDetector_sptr = boost::shared_ptr<IDetector>;
/// Shared pointer to an const IDetector object
using IDetector_const_sptr = boost::shared_ptr<const IDetector>;
/// unique pointer to an IDetector
using IDetector_uptr = std::unique_ptr<IDetector>;
/// unique pointer to an IDetector (const version)
using IDetector_const_uptr = std::unique_ptr<const IDetector>;

} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_IDETECTOR_FWD_H_

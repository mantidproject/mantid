#ifndef MANTID_API_PROJECTION_H_
#define MANTID_API_PROJECTION_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/V3D.h"

#include <stdexcept>

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {

/** Represents 3 dimensional projections

  @author Harry Jeffery
  @date 2015-02-5

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/// The units for a given dimension
enum ProjectionUnit {
  RLU,    // r.l.u
  INV_ANG // inverse angstroms
};

class DLLExport Projection {
public:
  /// Default constructor builds identity projection
  Projection();
  /// Three dimensional value constructor, w is the cross product of u and v.
  Projection(const Kernel::V3D &u, const Kernel::V3D &v);
  /// Three dimensional value constructor
  Projection(const Kernel::V3D &u, const Kernel::V3D &v, const Kernel::V3D &w);
  /// Construct from an ITableWorkspace
  Projection(const ITableWorkspace &ws);
  /// Destructor
  virtual ~Projection() = default;
  /// Retrieves the offset for the given dimension
  double getOffset(size_t nd);
  /// Retrieves the axis vector for the given dimension
  Kernel::V3D getAxis(size_t nd);
  /// Retrives the unit of the given dimension
  ProjectionUnit getUnit(size_t nd);
  /// Set the offset for a given dimension
  void setOffset(size_t nd, double offset);
  /// Set the axis vector for a given dimension
  void setAxis(size_t nd, Kernel::V3D axis);
  /// Set the unit for a given dimension
  void setUnit(size_t nd, ProjectionUnit unit);

  Kernel::V3D &U() { return m_dimensions[0]; }
  Kernel::V3D &V() { return m_dimensions[1]; }
  Kernel::V3D &W() { return m_dimensions[2]; }

protected:
  /// The dimensions
  Kernel::V3D m_dimensions[3];
  /// The offsets for each dimension
  double m_offsets[3];
  /// The units for each dimension
  ProjectionUnit m_units[3];
};

using Projection_sptr = boost::shared_ptr<Projection>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROJECTION_H_*/

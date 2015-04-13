#ifndef MANTID_API_PROJECTION_H_
#define MANTID_API_PROJECTION_H_

#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/V3D.h"

#include <stdexcept>

#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;

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
  Projection(const V3D &u, const V3D &v);
  /// Three dimensional value constructor
  Projection(const V3D &u, const V3D &v, const V3D &w);
  /// Construct from an ITableWorkspace
  Projection(const ITableWorkspace &ws);
  /// Copy constructor
  Projection(const Projection &other);
  /// Assignment operator
  Projection &operator=(const Projection &other);
  /// Destructor
  virtual ~Projection();
  /// Retrieves the offset for the given dimension
  double getOffset(size_t nd);
  /// Retrieves the axis vector for the given dimension
  V3D getAxis(size_t nd);
  /// Retrives the unit of the given dimension
  ProjectionUnit getUnit(size_t nd);
  /// Set the offset for a given dimension
  void setOffset(size_t nd, double offset);
  /// Set the axis vector for a given dimension
  void setAxis(size_t nd, V3D axis);
  /// Set the unit for a given dimension
  void setUnit(size_t nd, ProjectionUnit unit);

  V3D &U() { return m_dimensions[0]; }
  V3D &V() { return m_dimensions[1]; }
  V3D &W() { return m_dimensions[2]; }

protected:
  /// The dimensions
  V3D m_dimensions[3];
  /// The offsets for each dimension
  double m_offsets[3];
  /// The units for each dimension
  ProjectionUnit m_units[3];
};

typedef boost::shared_ptr<Projection> Projection_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROJECTION_H_*/

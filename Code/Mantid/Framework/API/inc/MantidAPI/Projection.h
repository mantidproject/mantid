#ifndef MANTID_API_PROJECTION_H_
#define MANTID_API_PROJECTION_H_

#include "MantidKernel/VMD.h"
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid {
namespace API {

/** Simple projection class for multiple dimensions (i.e. > 3).

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
  /// Default constructor builds with two dimensions
  Projection();
  /// Constructor specifying the number of dimensions
  Projection(size_t nd);
  /// Two dimensional value constructor
  Projection(VMD u, VMD v);
  /// Three dimensional value constructor
  Projection(VMD u, VMD v, VMD w);
  /// Four dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x);
  /// Five dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x, VMD y);
  /// Six dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x, VMD y, VMD z);
  /// Copy constructor
  Projection(const Projection &other);
  /// Assignment operator
  Projection &operator=(const Projection &other);
  /// Destructor
  virtual ~Projection();
  /// Retrieves the offset for the given dimension
  float getOffset(size_t nd);
  /// Retrieves the axis vector for the given dimension
  VMD getAxis(size_t nd);
  /// Retrives the unit of the given dimension
  ProjectionUnit getUnit(size_t nd);
  /// Set the offset for a given dimension
  void setOffset(size_t nd, float offset);
  /// Set the axis vector for a given dimension
  void setAxis(size_t nd, VMD axis);
  /// Set the unit for a given dimension
  void setUnit(size_t nd, ProjectionUnit unit);
  /// Retrives the number of dimensions
  size_t getNumDims() const { return m_nd; }

  // We're guaranteed to have at least 2 axes
  VMD &U() { return m_dimensions[0]; }
  VMD &V() { return m_dimensions[1]; }
  VMD &W() {
    if (m_nd >= 3)
      return m_dimensions[2];
    else
      throw std::invalid_argument("invalid axis");
  }

protected:
  /// Number of dimensions
  size_t m_nd;
  /// A vector of the dimensions
  VMD *m_dimensions;
  /// A vector of the offsets for each dimension
  float *m_offsets;
  /// A vector of the units for each dimension
  ProjectionUnit *m_units;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROJECTION_H_*/

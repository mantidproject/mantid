#ifndef MANTID_GEOMETRY_PROJECTION_H_
#define MANTID_GEOMETRY_PROJECTION_H_

#include "MantidKernel/VMD.h"
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

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

class DLLExport Projection {
public:
  /// Default constructor builds with one dimension
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
  float &getOffset(size_t nd);
  /// Retrieves the axis vector for the given dimension
  VMD &getAxis(size_t nd);

  VMD &U() { return getAxis(0); }
  VMD &V() { return getAxis(1); }
  VMD &W() { return getAxis(2); }

  size_t getNumDims() const { return m_nd; }

protected:
  /// Number of dimensions
  size_t m_nd;
  /// A vector of the dimensions
  VMD *m_dimensions;
  /// A vector of the offsets for each dimension
  float *m_offsets;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PROJECTION_H_*/

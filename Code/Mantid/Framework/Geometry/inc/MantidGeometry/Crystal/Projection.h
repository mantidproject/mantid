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
  Projection()
      : m_nd(1), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = VMD(m_nd);
    m_offsets[0] = 0.0;
  }
  /// Constructor specifying the number of dimensions
  Projection(size_t nd) : m_nd(nd) {
    if (m_nd <= 0)
      throw std::invalid_argument("nd must be > 0");

    m_dimensions = new VMD[m_nd];
    m_offsets = new float[m_nd];

    for (size_t i = 0; i < m_nd; ++i) {
      m_dimensions[i] = VMD(m_nd);
      m_offsets[i] = 0.0;
    }
  }
  /// Two dimensional value constructor
  Projection(VMD u, VMD v)
      : m_nd(2), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = u;
    m_dimensions[1] = v;
    m_offsets[0] = 0.0;
    m_offsets[1] = 0.0;
  }
  /// Three dimensional value constructor
  Projection(VMD u, VMD v, VMD w)
      : m_nd(3), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = u;
    m_dimensions[1] = v;
    m_dimensions[2] = w;
    for (size_t i = 0; i < m_nd; ++i)
      m_offsets[i] = 0.0;
  }
  /// Four dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x)
      : m_nd(4), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = u;
    m_dimensions[1] = v;
    m_dimensions[2] = w;
    m_dimensions[3] = x;
    for (size_t i = 0; i < m_nd; ++i)
      m_offsets[i] = 0.0;
  }
  /// Five dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x, VMD y)
      : m_nd(5), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = u;
    m_dimensions[1] = v;
    m_dimensions[2] = w;
    m_dimensions[3] = x;
    m_dimensions[4] = y;
    for (size_t i = 0; i < m_nd; ++i)
      m_offsets[i] = 0.0;
  }
  /// Six dimensional value constructor
  Projection(VMD u, VMD v, VMD w, VMD x, VMD y, VMD z)
      : m_nd(6), m_dimensions(new VMD[m_nd]), m_offsets(new float[m_nd]) {
    m_dimensions[0] = u;
    m_dimensions[1] = v;
    m_dimensions[2] = w;
    m_dimensions[3] = x;
    m_dimensions[4] = y;
    m_dimensions[5] = z;
    for (size_t i = 0; i < m_nd; ++i)
      m_offsets[i] = 0.0;
  }

  /// Copy constructor
  Projection(const Projection &other) : m_nd(other.m_nd) {
    if (m_nd <= 0)
      throw std::invalid_argument("nd must be > 0");
    m_dimensions = new VMD[m_nd];
    m_offsets = new float[m_nd];
    for (size_t i = 0; i < m_nd; ++i) {
      m_dimensions[i] = other.m_dimensions[i];
      m_offsets[i] = other.m_offsets[i];
    }
  }

  /// Assignment operator
  Projection &operator=(const Projection &other) {
    if (m_nd != other.m_nd) {
      m_nd = other.m_nd;
      delete[] m_dimensions;
      delete[] m_offsets;
      m_dimensions = new VMD[m_nd];
      m_offsets = new float[m_nd];
    }
    for (size_t i = 0; i < m_nd; ++i) {
      m_dimensions[i] = other.m_dimensions[i];
      m_offsets[i] = other.m_offsets[i];
    }
    return *this;
  }

  /// Destructor
  virtual ~Projection() {
    delete[] m_dimensions;
    delete[] m_offsets;
  }

  /// Retrieves the offset for the given dimension
  float &getOffset(size_t nd) {
    if (nd >= m_nd)
      throw std::invalid_argument("given axis out of range");
    else
      return m_offsets[nd];
  }

  /// Retrieves the axis vector for the given dimension
  VMD &getAxis(size_t nd) {
    if (nd >= m_nd)
      throw std::invalid_argument("given axis out of range");
    else
      return m_dimensions[nd];
  }

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

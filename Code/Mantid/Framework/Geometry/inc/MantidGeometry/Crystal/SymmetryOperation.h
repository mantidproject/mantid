#ifndef MANTID_GEOMETRY_SYMMETRYOPERATION_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATION_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{

/** SymmetryOperation :

    Crystallographic symmetry operations that involve rotations, roto-inversions
    and mirror-planes in three dimensions can be represented by 3x3 integer
    matrices.

    In this interface, each symmetry operation has an "order", which is an
    unsigned integer describing the number of times a symmetry operation
    has to be applied to an object until it is identical.

    This supplies one criterion for correctness-testing. Multiplying a vector
    n times with the corresponding matrix (where n = order) must result
    in a vector that is identical to the original one.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 25/07/2014

    Copyright © 2014 PSI-MSS

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
class MANTID_GEOMETRY_DLL SymmetryOperation
{
public:
    SymmetryOperation(size_t order, Kernel::IntMatrix matrix);

    virtual ~SymmetryOperation() { }

    size_t order() const;

    template<typename T>
    T apply(const T &operand) const
    {
        return m_matrix * operand;
    }

protected:
    size_t m_order;
    Kernel::IntMatrix m_matrix;
};

typedef boost::shared_ptr<SymmetryOperation> SymmetryOperation_sptr;
typedef boost::shared_ptr<const SymmetryOperation> SymmetryOperation_const_sptr;

class MANTID_GEOMETRY_DLL SymOpIdentity : public SymmetryOperation
{
public:
    SymOpIdentity();

};

class MANTID_GEOMETRY_DLL SymOpInversion : public SymmetryOperation
{
public:
    SymOpInversion();

};

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_SYMMETRYOPERATION_H_ */

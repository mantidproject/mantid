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

    Crystallographic symmetry operations that involve rotations, (roto-)inversions
    and mirror-planes in three dimensions can be represented by 3x3 integer
    matrices.

    In this interface, each symmetry operation has an "order", which is an
    unsigned integer describing the number of times a symmetry operation
    has to be applied to an object until it is identical.

    Furthermore, each symmetry operation has a string-identifier. It contains the
    symbol of the operation and the relevant direction, i.e. direction of a rotation
    axis or direction perpendicular to a mirror plane. Examples are "2 [100]" for
    a 2-fold rotation around the x-axis or "m [001]" for a mirror plane perpendicular
    to the z-axis. For hexagonal coordinate systems the symmetry operations differ,
    so their symbols are marked with an additional "h" at the end. One example is
    "2 [100]h" which denotes a 2-fold axis in x-direction of a hexagonal coordinate
    system. The matrices and identifiers are taken from [1].

    Using the symmetry operations in code is easy. All that is required is constructing
    an instance of the desired operation and calling its templated apply-method:

        SymOpMirrorPlaneZ symOp;
        V3D mirrored = symOp.apply(V3D(1, 1, 1));

    Because the symmetry operation is using Kernel::IntMatrix internally, it can be
    used on any object for which Kernel::IntMatrix implements a multiplication-operator.

    While all the operations could be represented by just one class (SymmetryOperation)
    with the correct parameters set, having one class for each operation provides more
    semantics in the code using these operations.

    References:
        [1] International Tables for Crystallography, Volume A, Fourth edition, pp 797-798.


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
    virtual ~SymmetryOperation() { }

    size_t order() const;
    std::string identifier() const;

    template<typename T>
    T apply(const T &operand) const
    {
        return m_matrix * operand;
    }

protected:
    SymmetryOperation(size_t order, Kernel::IntMatrix matrix, std::string identifier);
    void setMatrixFromArray(int array[]);

    size_t m_order;
    Kernel::IntMatrix m_matrix;
    std::string m_identifier;
};

typedef boost::shared_ptr<SymmetryOperation> SymmetryOperation_sptr;
typedef boost::shared_ptr<const SymmetryOperation> SymmetryOperation_const_sptr;

// Identity
class MANTID_GEOMETRY_DLL SymOpIdentity : public SymmetryOperation
{
public:
    SymOpIdentity();
};

// Inversion
class MANTID_GEOMETRY_DLL SymOpInversion : public SymmetryOperation
{
public:
    SymOpInversion();
};

// Rotations 2-fold
// x-axis
class MANTID_GEOMETRY_DLL SymOpRotationTwoFoldX : public SymmetryOperation
{
public:
    SymOpRotationTwoFoldX();
};

// y-axis
class MANTID_GEOMETRY_DLL SymOpRotationTwoFoldY : public SymmetryOperation
{
public:
    SymOpRotationTwoFoldY();
};

// z-axis
class MANTID_GEOMETRY_DLL SymOpRotationTwoFoldZ : public SymmetryOperation
{
public:
    SymOpRotationTwoFoldZ();
};

// x-axis
class MANTID_GEOMETRY_DLL SymOpRotationTwoFoldXHexagonal : public SymmetryOperation
{
public:
    SymOpRotationTwoFoldXHexagonal();
};

// 210, hexagonal
class MANTID_GEOMETRY_DLL SymOpRotationTwoFold210Hexagonal : public SymmetryOperation
{
public:
    SymOpRotationTwoFold210Hexagonal();
};

// Rotations 4-fold
// z-axis
class MANTID_GEOMETRY_DLL SymOpRotationFourFoldZ : public SymmetryOperation
{
public:
    SymOpRotationFourFoldZ();
};

// Rotations 3-fold
// z-axis, hexagonal
class MANTID_GEOMETRY_DLL SymOpRotationThreeFoldZHexagonal : public SymmetryOperation
{
public:
    SymOpRotationThreeFoldZHexagonal();
};

// 111
class MANTID_GEOMETRY_DLL SymOpRotationThreeFold111 : public SymmetryOperation
{
public:
    SymOpRotationThreeFold111();
};

// Rotations 6-fold
// z-axis, hexagonal
class MANTID_GEOMETRY_DLL SymOpRotationSixFoldZHexagonal : public SymmetryOperation
{
public:
    SymOpRotationSixFoldZHexagonal();
};

// Mirror planes
// y-axis
class MANTID_GEOMETRY_DLL SymOpMirrorPlaneY : public SymmetryOperation
{
public:
    SymOpMirrorPlaneY();
};

// z-axis
class MANTID_GEOMETRY_DLL SymOpMirrorPlaneZ : public SymmetryOperation
{
public:
    SymOpMirrorPlaneZ();
};

// 210, hexagonal
class MANTID_GEOMETRY_DLL SymOpMirrorPlane210Hexagonal : public SymmetryOperation
{
public:
    SymOpMirrorPlane210Hexagonal();
};


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_SYMMETRYOPERATION_H_ */

#include "MantidGeometry/Crystal/SymmetryOperation.h"

namespace Mantid
{
namespace Geometry
{

// SymmertryOperation - base class
SymmetryOperation::SymmetryOperation(size_t order, Kernel::IntMatrix matrix, std::string identifier) :
    m_order(order),
    m_matrix(matrix),
    m_identifier(identifier)
{
}

size_t SymmetryOperation::order() const
{
    return m_order;
}

std::string SymmetryOperation::identifier() const
{
    return m_identifier;
}

void SymmetryOperation::setMatrixFromArray(int array[])
{
    for(size_t row = 0; row < 3; ++row) {
        for(size_t col = 0; col < 3; ++col) {
            m_matrix[row][col] = array[row * 3 + col];
        }
    }
}

// Identity
SymOpIdentity::SymOpIdentity() :
    SymmetryOperation(1, Kernel::IntMatrix(3, 3, true), "1")
{

}

// Inversion
SymOpInversion::SymOpInversion() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3, true), "-1")
{
    m_matrix *= -1;
}

// Rotations 2-fold
// x-axis
SymOpRotationTwoFoldX::SymOpRotationTwoFoldX() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]")
{
    int rotTwoFoldX[] = {1,  0,  0,
                         0, -1,  0,
                         0,  0, -1};

    setMatrixFromArray(rotTwoFoldX);
}

// y-axis
SymOpRotationTwoFoldY::SymOpRotationTwoFoldY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [010]")
{
    int rotTwoFoldY[] = {-1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(rotTwoFoldY);
}

// z-axis
SymOpRotationTwoFoldZ::SymOpRotationTwoFoldZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [001]")
{
    int rotTwoFoldZ[] = {-1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(rotTwoFoldZ);
}

// x-axis, hexagonal
SymOpRotationTwoFoldXHexagonal::SymOpRotationTwoFoldXHexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]h")
{
    int rotTwoFoldXHexagonal[] = {1, -1,  0,
                                  0, -1,  0,
                                  0,  0, -1};

    setMatrixFromArray(rotTwoFoldXHexagonal);
}

// 210, hexagonal
SymOpRotationTwoFold210Hexagonal::SymOpRotationTwoFold210Hexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [210]h")
{
    int rotTwoFold210Hexagonal[] = { 1,  0,  0,
                                     1, -1,  0,
                                     0,  0, -1};

    setMatrixFromArray(rotTwoFold210Hexagonal);
}

// Rotations 4-fold
// z-axis
SymOpRotationFourFoldZ::SymOpRotationFourFoldZ() :
    SymmetryOperation(4, Kernel::IntMatrix(3, 3), "4 [001]")
{
    int rotFourFoldZ[] = { 0, -1,  0,
                           1,  0,  0,
                           0,  0,  1};

    setMatrixFromArray(rotFourFoldZ);
}

// Rotations 3-fold
// z-axis, hexagonal
SymOpRotationThreeFoldZHexagonal::SymOpRotationThreeFoldZHexagonal() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [001]h")
{
    int rotThreeFoldZHexagonal[] = { 0, -1,  0,
                                     1, -1,  0,
                                     0,  0,  1};

    setMatrixFromArray(rotThreeFoldZHexagonal);
}

SymOpRotationThreeFold111::SymOpRotationThreeFold111() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [111]")
{
    int rotThreeFold111[] = { 0,  0,  1,
                              1,  0,  0,
                              0,  1,  0};

    setMatrixFromArray(rotThreeFold111);
}

// Rotations 6-fold
// z-axis, hexagonal
SymOpRotationSixFoldZHexagonal::SymOpRotationSixFoldZHexagonal() :
    SymmetryOperation(6, Kernel::IntMatrix(3, 3), "6 [001]h")
{
    int rotSixFoldZHexagonal[] = { 1, -1,  0,
                                   1,  0,  0,
                                   0,  0,  1};

    setMatrixFromArray(rotSixFoldZHexagonal);
}

// Mirror planes
SymOpMirrorPlaneY::SymOpMirrorPlaneY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [010]")
{
    int mirrorPlaneY[] = {1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(mirrorPlaneY);
}

SymOpMirrorPlaneZ::SymOpMirrorPlaneZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [001]")
{
    int mirrorPlaneZ[] = {1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(mirrorPlaneZ);
}

SymOpMirrorPlane210Hexagonal::SymOpMirrorPlane210Hexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [210]h")
{
    int mirrorPlane210Hexagonal[] = {-1,  0,  0,
                                     -1,  1,  0,
                                      0,  0,  1};

    setMatrixFromArray(mirrorPlane210Hexagonal);
}

} // namespace Geometry
} // namespace Mantid

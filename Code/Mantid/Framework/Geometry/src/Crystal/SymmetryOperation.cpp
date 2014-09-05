#include "MantidGeometry/Crystal/SymmetryOperation.h"

namespace Mantid
{
namespace Geometry
{

/**
 * The constructor of SymmetryOperation
 *
 * Since the SymmetryOperation base-class is not intended to be used directly, the
 * only constructor is protected. This way, it can be called by the inheriting classes
 * with the correct parameters, but no "anonymous" symmetry operation is possible.
 *
 * Three parameters are required. The "order", as described in the header file specifies
 * how often the operation has to be applied to an object in sequence until it is
 * identical with itself. The matrix-parameter is the transformation matrix that
 * represents the operation and identifier is a string that follows a certain convention,
 * also described in the header file. It must match the following regular expression:
 *
 *      ^-?((1)|((2|3|4|6|m) \\[(-?\\d{1}){3}\\]h?))$
 *
 * @param order :: Order of the symmetry operation.
 * @param matrix :: Integer matrix with dimensions 3x3, defines symmetry operation.
 * @param identifier :: Identifier string for symmetry operation.
 */
SymmetryOperation::SymmetryOperation(size_t order, Kernel::IntMatrix matrix, std::string identifier) :
    m_order(order),
    m_matrix(matrix),
    m_identifier(identifier)
{
}

/**
 * Returns the order of the symmetry operation
 *
 * @return Order of the symmetry operation
 */
size_t SymmetryOperation::order() const
{
    return m_order;
}

/**
 * Returns the string-identifier for this symmetry operation
 *
 * @return Identifier of the symmetry operation
 */
std::string SymmetryOperation::identifier() const
{
    return m_identifier;
}

/**
 * Takes a flat int-array and assigns its 9 elements to the internal matrix.
 *
 * @param array :: int-array containing the transformation matrix.
 */
void SymmetryOperation::setMatrixFromArray(int array[])
{
    for(size_t row = 0; row < 3; ++row) {
        for(size_t col = 0; col < 3; ++col) {
            m_matrix[row][col] = array[row * 3 + col];
        }
    }
}

/// Identity
SymOpIdentity::SymOpIdentity() :
    SymmetryOperation(1, Kernel::IntMatrix(3, 3, true), "1")
{

}

/// Inversion
SymOpInversion::SymOpInversion() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3, true), "-1")
{
    m_matrix *= -1;
}

/* 2-fold rotation axes */
/// 2-fold rotation around x-axis
SymOpRotationTwoFoldX::SymOpRotationTwoFoldX() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]")
{
    int rotTwoFoldX[] = {1,  0,  0,
                         0, -1,  0,
                         0,  0, -1};

    setMatrixFromArray(rotTwoFoldX);
}

/// 2-fold rotation around y-axis
SymOpRotationTwoFoldY::SymOpRotationTwoFoldY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [010]")
{
    int rotTwoFoldY[] = {-1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(rotTwoFoldY);
}

/// 2-fold rotation around z-axis
SymOpRotationTwoFoldZ::SymOpRotationTwoFoldZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [001]")
{
    int rotTwoFoldZ[] = {-1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(rotTwoFoldZ);
}

/// 2-fold rotation around x-axis, hexagonal coordinate system
SymOpRotationTwoFoldXHexagonal::SymOpRotationTwoFoldXHexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]h")
{
    int rotTwoFoldXHexagonal[] = {1, -1,  0,
                                  0, -1,  0,
                                  0,  0, -1};

    setMatrixFromArray(rotTwoFoldXHexagonal);
}

/// 2-fold rotation around [210]-axis, hexagonal coordinate system
SymOpRotationTwoFold210Hexagonal::SymOpRotationTwoFold210Hexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [210]h")
{
    int rotTwoFold210Hexagonal[] = { 1,  0,  0,
                                     1, -1,  0,
                                     0,  0, -1};

    setMatrixFromArray(rotTwoFold210Hexagonal);
}

/* 4-fold rotation axes */
/// 4-fold rotation around z-axis
SymOpRotationFourFoldZ::SymOpRotationFourFoldZ() :
    SymmetryOperation(4, Kernel::IntMatrix(3, 3), "4 [001]")
{
    int rotFourFoldZ[] = { 0, -1,  0,
                           1,  0,  0,
                           0,  0,  1};

    setMatrixFromArray(rotFourFoldZ);
}

/* 3-fold rotation axes */
/// 3-fold rotation around z-axis, hexagonal coordinate system
SymOpRotationThreeFoldZHexagonal::SymOpRotationThreeFoldZHexagonal() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [001]h")
{
    int rotThreeFoldZHexagonal[] = { 0, -1,  0,
                                     1, -1,  0,
                                     0,  0,  1};

    setMatrixFromArray(rotThreeFoldZHexagonal);
}

/// 3-fold rotation around [111]-axis
SymOpRotationThreeFold111::SymOpRotationThreeFold111() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [111]")
{
    int rotThreeFold111[] = { 0,  0,  1,
                              1,  0,  0,
                              0,  1,  0};

    setMatrixFromArray(rotThreeFold111);
}

/* 6-fold rotation axes */
/// 6-fold rotation around z-axis, hexagonal coordinate system
SymOpRotationSixFoldZHexagonal::SymOpRotationSixFoldZHexagonal() :
    SymmetryOperation(6, Kernel::IntMatrix(3, 3), "6 [001]h")
{
    int rotSixFoldZHexagonal[] = { 1, -1,  0,
                                   1,  0,  0,
                                   0,  0,  1};

    setMatrixFromArray(rotSixFoldZHexagonal);
}

/* Mirror planes */
/// Mirror plane perpendicular to y-axis
SymOpMirrorPlaneY::SymOpMirrorPlaneY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [010]")
{
    int mirrorPlaneY[] = {1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(mirrorPlaneY);
}

/// Mirror plane perpendicular to z-axis
SymOpMirrorPlaneZ::SymOpMirrorPlaneZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [001]")
{
    int mirrorPlaneZ[] = {1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(mirrorPlaneZ);
}

/// Mirror plane perpendicular to [210]-axis
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

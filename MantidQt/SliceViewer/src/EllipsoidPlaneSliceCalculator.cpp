#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidKernel/V2D.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>

/**
 *
 * The functions in this file intend to calculate the paramters of an ellipse
 *which
 * is obtained when slicing an arbitrary ellipsoid with a plane which runs
 *parallel
 * to the x-y plane.
 *
 * The following should serve as a motivation of the derivation.
 *
 * General Ellipsoid Equation:
 * Transpose[x-x0]*M*(x-x0) = 1
 *
 * Note that the ellipsoid matrix is symmetric and positive semi-definite.
 *
 * Taking the cut:
 * The plane is at z = zp
 *
 * We transform: x = x-x0 , y = y-y0, zk= zp -z0
 *
 * This results in the expression:
 *
 * m00x^2 + 2*m01*x*y + 2*m02*zk*x + m11*y^2 + 2*m12*zk*y + m22*zk = 1
 *
 * or in matrix form
 *
 *
 * Transpose[Q]*A*Q  + Transpose[B]*Q + c = 1
 *
 * with:
 * Q = (x)
 *     (y)
 *
 * A = (m00  m01)
 *     (m01  m11)
 *
 * B = 2*zk *(m02)
 *           (m12)
 *
 * c = m22*zk
 * ---------------------------
 * Using quadratic completion we can get rid of (rather refactor) the linear
 *term:
 * (Note we make use that A = Transpose[A]
 *
 * Transpose[(Q-K)]*A*(Q-K) + Transpose[B + 2AK]*Q - Transpose[K]*A*Transpose[K]
 *+ c = 1
 *
 * K is set to -A^(-1)*B/2
 *
 * which leads to the standard form for the 2D ellipse:
 *
 * Transpose[Q + A^(-1)*B/2] * A/(Transpose[B]*A^(-1)*B/4 - (c-1)) *(Q +
 *A^(-1)*B/2) = 1
 *
 *
 * The ellipse equation
 * MM = A/(Transpose[B]*A^(-1)*B/4 - (c-1))
 * can be used to obtain the ellipse radii and (directions if needed)
 *
 * The ellipse origin is
 * -A^(-1)*B/2
 *
 * The tilting angle is:
 * alpha = -atan(2*a01/(a11-a00))/2
 *
 */

namespace
{

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
almost_equal(T x, T y)
{
    // TODO: Add ULP
    return std::abs(x - y) < std::numeric_limits<T>::epsilon() * std::abs(x + y)
           || std::abs(x - y) < std::numeric_limits<T>::min();
}

/**
 * Calcualtes the origin of the ellipse.  The originis defined by -A^(-1)*B/2.
 * @param AInverse: the inverse of the A matrix (see above)
 * @param B: the b vector
 * @param originEllipsoid: the origin of the ellipsoid
 * @param zVal: the z value (in the ellipse frame)
 * @return the origin of the ellipse
 */
Mantid::Kernel::V3D getOrigin(Mantid::Kernel::DblMatrix AInverse,
                              Mantid::Kernel::DblMatrix B,
                              Mantid::Kernel::V3D originEllipsoid, double zVal)
{
    const auto multiplied = AInverse * B;

    // Apply the -0.5 factor and shift back into the ellipsoid frame
    const auto x = -0.5 * multiplied[0][0] + originEllipsoid.X();
    const auto y = -0.5 * multiplied[1][0] + originEllipsoid.Y();
    const auto z = zVal + originEllipsoid.Z();
    return Mantid::Kernel::V3D(x, y, z);
}

#if 0

/**
 * Calcualtion ofthe eigenvectors
 * @param evMinorAxis: the minor axis eigen value
 * @param evMajorAxis: the major axis eigen value
 * @param MM: the MM Matrix
 * @return the eigen vectors
 */
std::pair<Mantid::Kernel::V2D, Mantid::Kernel::V2D>
getEigenDirections(double evMinorAxis, double evMajorAxis,
                   Mantid::Kernel::DblMatrix MM)
{
  // Calcualte the eigen vector of the minor axis
  auto isM00SmallerM11 = MM[0][0] - M[1][1];

  Mantid::Kernel::V2D eigenVectorMinorAxis;
  if (isM00SmallerM11) {
      eigenVectorMinorAxis[0] = MM[0][1];
      eigenVectorMinorAxis[1]= evMinorAxis - M[0][0];
  } else {
      eigenVectorMinorAxis[0] = evMinorAxis - M[1][1];
      eigenVectorMinorAxis[1] = M[0][1];
  }
  auto normMinorAxis = eigenVectorMinorAxis.norm();
  eigenVectorMinorAxis /= normMinorAxis;

  // Calculate the eigen vector of the major axis
  Mantid::Kernel::V2D eigenVectorMajorAxis;
  eigenVectorMajorAxis[0] = -eigenVectorMinorAxis[1];
  eigenVectorMajorAxis[1] = eigenVectorMinorAxis[0];

  return std::make_pair(eigenVectorMinorAxis, eigenVectorMajorAxis);
}
#endif
/**
 * Calculates the Radii and the directions of th
 * @param A: the A matrix
 * @param AInverse: the inverse of the A matrix
 * @param B: the B vector
 * @param BT: the transposed B vector
 * @param c: the c shift factor
 * @return radii and directions
 */
std::pair<double, double> getAxesInformation(Mantid::Kernel::DblMatrix A,
                                   Mantid::Kernel::DblMatrix AInverse,
                                   Mantid::Kernel::DblMatrix B,
                                   Mantid::Kernel::DblMatrix BT, double c)
{
    // Calculate the denominator: (Transpose[B]*A^(-1)*B/4 - (c-1))
    const auto temp1 = AInverse * B;
    const auto temp2 = BT * temp1;
    const auto denominator = 0.25 * temp2[0][0] - c + 1;

    // Calculate the MM matrix: A/(Transpose[B]*A^(-1)*B/4 - (c-1))
    auto MM = A;
    MM /= denominator;

    // Calculate the Eigenvalues: since we are dealing with EVs of a
    // 2x2, symmetric and positive semi-definite matrix we can
    // just write down the result of the EV calculation and save time
    // EV = (MM00 + MM11 +/- Sqrt[(MM00-M11)^2 + 4(MM01)^2])/2
    const auto evPart1 = MM[0][0] + MM[1][1];
    const auto evPart2 = std::sqrt(std::pow(MM[0][0] - MM[1][1], 2)
                                   + 4 * std::pow(MM[0][1], 2));

    const auto evMinorAxis = (evPart1 + evPart2) * 0.5;
    const auto evMajorAxis = (evPart1 - evPart2) * 0.5;

    // Get the radii: they are just the squareroot of the inverse of teh EV
    const auto radiusMinorAxis = 1 / std::sqrt(evMinorAxis);
    const auto radiusMajorAxis = 1 / std::sqrt(evMajorAxis);

    // Note that we don't have to perform any transformations on the radius, as
    // they will
    // not be affected by a translation
    return std::make_pair(radiusMinorAxis, radiusMajorAxis);
}

/**
 * Calculates the angle between x in the original frame and the x' in the
 *ellipse frame
 * @param A: the A matrix
 * @return the angle between x and x'
 *
 * Note that this is: -atan(2*a01/(a11-a00))/2
 */
double getAngle(Mantid::Kernel::DblMatrix A)
{
    const auto factor = 2 * A[0][1] / (A[1][1] - A[0][0]);
    return -0.5 * std::atan(factor);
}
}

namespace Mantid
{
namespace SliceViewer
{

SliceEllipseInfo EllipsoidPlaneSliceCalculator::getSlicePlaneInfo(
    std::vector<Mantid::Kernel::V3D> directions, std::vector<double> radii,
    Mantid::Kernel::V3D originEllipsoid, double zPlane)
{
    // Setup the Ellipsoid Matrix
    auto m = createEllipsoidMatrixInXYZFrame(directions, radii);

    auto isEllipsoid = checkIfIsEllipse(m);

    if (isEllipsoid) {
        return getSolutionForEllipsoid(m, zPlane, originEllipsoid);
    } else {
        throw std::runtime_error("EllipsoidPlaneSliceCalcualtor: The peak does "
                                 "not seem to create an elliptical or "
                                 "spherical cut.");
    }
}

/**
 * Check if we are dealing with an ellipsoid: m00*m11 - m01^2 > 0 must be
 * fullfilled
 * @param m: the ellipsoid matrix
 * @return true if we are dealing with a true ellipsoid
 */
bool EllipsoidPlaneSliceCalculator::checkIfIsEllipse(
    const Kernel::Matrix<double> &m) const
{
    auto isEllipse = m[0][0] * m[1][1] - std::pow(m[0][1], 2);
    return isEllipse;
}

/**
 * Check if we are dealing with a circle: m00 == m11 and  m01 == 0
 * @param m: the ellipsoid matrix
 * @return true if we are dealing with circle
 */
bool EllipsoidPlaneSliceCalculator::checkIfIsCircle(const Mantid::Kernel::Matrix<double> &m) const
{
    auto isM00EqualM11 = almost_equal(m[0][0], m[1][1]);
    auto isM01Zero = almost_equal(m[0][1], 0.0);
    return isM00EqualM11 && isM01Zero;
}


SliceEllipseInfo EllipsoidPlaneSliceCalculator::getSolutionForEllipsoid(
    const Kernel::Matrix<double> &m, double zPlane,
    Mantid::Kernel::V3D originEllipsoid) const
{
    // Shift the z value into a suitable frame
    const double z = zPlane - originEllipsoid.Z();

    // Setup the A matrix
    Mantid::Kernel::DblMatrix A;
    A.setMem(2, 2);
    const std::vector<double> ARow0 = {m[0][0], m[0][1]};
    const std::vector<double> ARow1 = {m[0][1], m[1][1]};
    A.setRow(0, ARow0);
    A.setRow(1, ARow1);

    // Setup the inverse Matrix of A
    Mantid::Kernel::DblMatrix AInverse;
    const double detA = A.determinant();
    AInverse.setMem(2, 2);
    const std::vector<double> AInverseRow0 = {m[1][1] / detA, -m[0][1] / detA};
    const std::vector<double> AInverseRow1 = {-m[0][1] / detA, m[0][0] / detA};
    AInverse.setRow(0, AInverseRow0);
    AInverse.setRow(1, AInverseRow1);

    // Setup the B vector
    Mantid::Kernel::DblMatrix B;
    std::vector<double> BColumn = {m[0][2], m[1][2]};
    B.setMem(2, 1);
    B.setColumn(0, BColumn);
    B *= 2 * z;

    // Setip the Transpose B vector
    Mantid::Kernel::DblMatrix BT;
    std::vector<double> BTRow = {m[0][2], m[1][2]};
    BT.setMem(1, 2);
    BT.setRow(0, BTRow);
    BT *= 2 * z;

    // Setup the C factor
    const double c = m[2][2] * std::pow(z, 2);

    // Get the origin
    const auto origin = getOrigin(AInverse, B, originEllipsoid, z);

    // Get the radii
    const auto radii = getAxesInformation(A, AInverse, B, BT, c);

    // Get angle. If we have a circle then the angle is 0 (we want to avoid a
    // divergence here)
    const auto isCircle = checkIfIsCircle(m);
    const double angle = isCircle ? 0.0 : getAngle(A);

    // Calculat the roation angle
    return SliceEllipseInfo(origin, radii.first, radii.second, angle);
}

/**
 * Creates an ellipsoid matrix in the xyz frame based on the directions and
 *radii
 * of the ellipsoid.
 * @param directions: the three ellipsoid directions
 * @param radii: the three ellipsoid radii
 * @return an ellipsoid matrix in the xyz frame
 *
 * The directions represent the eigen directions of the ellipsoid expressed in
 * the coordinates of a xyz basis. The Ellipsoid matrix in the basis
 * representation of the eigendirections is
 *
 * 1/r1^1   0      0
 *   0     1/r2^2  0
 *   0      0     1/r3^2
 *
 *
 * We want the representation of this matrix in in the xyz basis. The general
 * equation for teh ellipsoid is Transpose[x]*M*x == 1. Change of basis
 * can be performed by  Transpose[x]*Transpose[S]*S*M*Transpose[S]*S*x
 * where S is a unitary matrix.
 *
 * See here for more info: https://en.wikipedia.org/wiki/Ellipsoid
 *                         https://en.wikipedia.org/wiki/Quadratic_form
 */
Mantid::Kernel::Matrix<double>
createEllipsoidMatrixInXYZFrame(std::vector<Mantid::Kernel::V3D> directions,
                                std::vector<double> radii)
{
    // Setup the transform matrix from the xyz system to the eigen vector
    // system, ie the directions
    auto vec0 = std::vector<double>(directions[0]);
    auto vec1 = std::vector<double>(directions[1]);
    auto vec2 = std::vector<double>(directions[2]);

    Mantid::Kernel::Matrix<double> s;
    Mantid::Kernel::Matrix<double> sTransposed;

    s.setMem(3, 3);
    sTransposed.setMem(3, 3);

    s.setColumn(0, vec0);
    s.setColumn(1, vec1);
    s.setColumn(2, vec2);

    sTransposed.setRow(0, vec0);
    sTransposed.setRow(1, vec1);
    sTransposed.setRow(2, vec2);

    // Setup the ellipsoid matrix in the eigenvector system, ie a unit matrix
    // with 1/ri^2 on the diagonal
    //
    Mantid::Kernel::Matrix<double> e;
    e.setMem(3, 3);
    std::vector<double> e0 = {1 / pow(radii[0], 2), 0.0, 0.0};
    std::vector<double> e1 = {0.0, 1 / pow(radii[1], 2), 0.0};
    std::vector<double> e2 = {0.0, 0.0, 1 / pow(radii[2], 2)};

    e.setRow(0, e0);
    e.setRow(1, e1);
    e.setRow(2, e2);
    // Now mulitply s*e*Transpose[s]
    return s * e * sTransposed;
}
}
}

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidKernel/V2D.h"

#include <algorithm>
#include <cmath>
/**
 *
 * The functions in this file intend to calculate the paramters of an ellipse
 *which
 * is obtained when slicing an arbitrary ellipsoid with a plane which runs
 *parallel
 * to the x-y plane. I was able to verify the calculations partially by
 *comparing
 * with the information provided here:
 *http://www.geometrictools.com/Documentation/InformationAboutEllipses.pdf.
 *
 * The following should serve as a motivation of the derivation.
 *
 * General Ellipsoid Equation (see here:
 *https://en.wikipedia.org/wiki/Ellipsoid#Generalised_equations)
 * Transpose[x-x0]*M*(x-x0) = 1
 *
 * Note that the ellipsoid matrix is symmetric and positive semi-definite.
 *
 * Taking the cut:
 * The plane is at z = zp. We just have to set z to this fixed value. This
 *leaves us with
 * an equation of the ellipse of the cut plane (provided a cut is possible).
 *
 * We transform: x = x-x0 , y = y-y0, zk= zp -z0 (ie we look at a center-origin
 *system)
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
 * Using quadratic completion we can get rid of ( or rather refactor) the linear
 * term:
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
 * The tilting angle is calcualted via the eigenvectors.
 * This is the angle required to rotate the axis into the major axis.
 *
 */

namespace {

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
                              Mantid::Kernel::V3D originEllipsoid,
                              double zVal) {
  const auto multiplied = AInverse * B;

  // Apply the -0.5 factor and shift back into the ellipsoid frame
  const auto x = -0.5 * multiplied[0][0] + originEllipsoid.X();
  const auto y = -0.5 * multiplied[1][0] + originEllipsoid.Y();
  const auto z = zVal + originEllipsoid.Z();
  return Mantid::Kernel::V3D(x, y, z);
}

struct EigenSystemEllipse {
  EigenSystemEllipse(Mantid::Kernel::V2D majorAxis,
                     Mantid::Kernel::V2D minorAxis, double majorRadius,
                     double minorRadius)
      : majorAxis(majorAxis), minorAxis(minorAxis), majorRadius(majorRadius),
        minorRadius(minorRadius) {}

  const Mantid::Kernel::V2D majorAxis;
  const Mantid::Kernel::V2D minorAxis;
  const double majorRadius;
  const double minorRadius;
};

/**
 * Get the eigenvalues for the ellipse
 * @param MM: the ellipse matrix
 * @param eigenValueMinor: the eigen value of the minor axis
 * @return the eigen vectors of the ellipse
 *
 * Note that we follow the procedure suggested here:
 *http://www.geometrictools.com/Documentation/InformationAboutEllipses.pdf.
 */
std::pair<Mantid::Kernel::V2D, Mantid::Kernel::V2D>
getEigenVectorsForEllipse(const Mantid::Kernel::DblMatrix &MM,
                          double eigenValueMinor) {
  auto isM00LargerThanM11 = MM[0][0] >= MM[1][1];

  // Create minor axis
  double minorX = 0.0;
  double minorY = 1.0;
  double norm = 1.0;
  if (isM00LargerThanM11) {
    minorX = eigenValueMinor - MM[1][1];
    minorY = MM[0][1];
    norm = std::sqrt(std::pow(minorX, 2) + std::pow(minorY, 2));
  } else {
    minorX = MM[0][1];
    minorY = eigenValueMinor - MM[0][0];
    norm = std::sqrt(std::pow(minorX, 2) + std::pow(minorY, 2));
  }
  minorX /= norm;
  minorY /= norm;
  Mantid::Kernel::V2D minorAxis(minorX, minorY);

  // Set the major axis
  Mantid::Kernel::V2D majorAxis(-minorAxis[1], minorAxis[0]);

  return std::make_pair(majorAxis, minorAxis);
}

/**
 * Calculates the Radii and the directions of th
 * @param A: the A matrix
 * @param AInverse: the inverse of the A matrix
 * @param B: the B vector
 * @param BT: the transposed B vector
 * @param c: the c shift factor
 * @return radii and directions
 */
EigenSystemEllipse getAxesInformation(Mantid::Kernel::DblMatrix A,
                                      Mantid::Kernel::DblMatrix AInverse,
                                      Mantid::Kernel::DblMatrix B,
                                      Mantid::Kernel::DblMatrix BT, double c) {
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
  const auto evPart2 =
      std::sqrt(std::pow(MM[0][0] - MM[1][1], 2) + 4 * std::pow(MM[0][1], 2));

  const auto evMinorAxis = (evPart1 + evPart2) * 0.5;
  const auto evMajorAxis = (evPart1 - evPart2) * 0.5;

  auto eigenVectors = getEigenVectorsForEllipse(MM, evMinorAxis);

  // Get the radii: they are just the squareroot of the inverse of teh EV
  const auto radiusMinorAxis = 1 / std::sqrt(evMinorAxis);
  const auto radiusMajorAxis = 1 / std::sqrt(evMajorAxis);

  // Note that we don't have to perform any transformations on the radius, as
  // they will not be affected by a translation
  return EigenSystemEllipse(eigenVectors.first, eigenVectors.second,
                            radiusMajorAxis, radiusMinorAxis);
}

/**
 * Angle which is required to rotate an axis aligned-ellipse (major axis along
 * x)
 * to the original major axis.
 * @param majorAxis: the eigen vector for the major axis
 * @return the angle required to rotate from the x axis to the major axis.
 *
 * Note that this could be solved by -atan2(2*a01/(a11-a00))/2 for angles up to
 * 45 degrees, but then atan fails and we loose quadrant information. Therefore
 *we need to
 * calculate the eigenvectors and get the angle from the components of the
 *eigenvectors.
 */
double getAngle(Mantid::Kernel::V2D majorAxis) {
  return std::atan2(majorAxis.Y(), majorAxis.X());
}

bool isBetweenEndpoints(double endpoint1, double endpoint2, double z) {
  const auto isBetween1And2 = (endpoint1 < z) && (z < endpoint2);
  const auto isBetween2And1 = (endpoint2 < z) && (z < endpoint1);

  return isBetween1And2 || isBetween2And1;
}
} // namespace

namespace Mantid {
namespace SliceViewer {

SliceEllipseInfo EllipsoidPlaneSliceCalculator::getSlicePlaneInfo(
    std::vector<Mantid::Kernel::V3D> directions, std::vector<double> radii,
    Mantid::Kernel::V3D originEllipsoid, double zPlane) const {
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
    const Kernel::Matrix<double> &m) const {
  auto isEllipse = (m[0][0] * m[1][1] - std::pow(m[0][1], 2)) > 0;
  return isEllipse;
}

/**
 * Check if we are dealing with a circle: m00 == m11 and  m01 == 0
 * @param m: the ellipsoid matrix
 * @return true if we are dealing with circle
 */
bool EllipsoidPlaneSliceCalculator::checkIfIsCircle(
    const Mantid::Kernel::Matrix<double> &m) const {
  auto isM00EqualM11 = Mantid::SliceViewer::almost_equal(m[0][0], m[1][1]);
  auto isM01Zero = Mantid::SliceViewer::almost_equal(m[0][1], 0.0);
  return isM00EqualM11 && isM01Zero;
}

SliceEllipseInfo EllipsoidPlaneSliceCalculator::getSolutionForEllipsoid(
    const Kernel::Matrix<double> &m, double zPlane,
    Mantid::Kernel::V3D originEllipsoid) const {
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

  // Get the radii + directions
  const auto eigenSystem = getAxesInformation(A, AInverse, B, BT, c);

  // Get angle. If we have a circle then the angle is 0 (we want to avoid a
  // divergence here)
  const auto isCircle = checkIfIsCircle(m);
  const double angle = isCircle ? 0.0 : getAngle(eigenSystem.majorAxis);

  return SliceEllipseInfo(origin, eigenSystem.majorRadius,
                          eigenSystem.minorRadius, angle);
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
 * where S is a unitary matrix. The ellipsoid matrix is S*M*Transpose[S].
 *
 * See here for more info: https://en.wikipedia.org/wiki/Ellipsoid
 *                         https://en.wikipedia.org/wiki/Quadratic_form
 */
Mantid::Kernel::Matrix<double>
createEllipsoidMatrixInXYZFrame(std::vector<Mantid::Kernel::V3D> directions,
                                std::vector<double> radii) {
  // Setup the transform matrix from the xyz system to the eigen vector
  // system, ie the directions
  auto vec0 = std::vector<double>(directions[0]);
  auto vec1 = std::vector<double>(directions[1]);
  auto vec2 = std::vector<double>(directions[2]);

  Mantid::Kernel::Matrix<double> s;
  Mantid::Kernel::Matrix<double> sTransposed;

  s.setMem(3, 3);
  sTransposed.setMem(3, 3);

  // The eigenvector is the column of the transformation matrix
  s.setColumn(0, vec0);
  s.setColumn(1, vec1);
  s.setColumn(2, vec2);

  sTransposed.setRow(0, vec0);
  sTransposed.setRow(1, vec1);
  sTransposed.setRow(2, vec2);

  // Setup the ellipsoid matrix in the eigenvector system, ie a unit matrix
  // with 1/ri^2 on the diagonal
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

/**
 * Check if a cut with the ellipsoid is possible at all
 * @param directions: the three ellipsoid directions
 * @param radii: the ellipsoid radii
 * @param originEllipsoid: the origin of the ellipsoid
 * @param zPlane: the z plane value
 * @return true if the a cut exists, else false
 */
bool checkIfCutExists(const std::vector<Mantid::Kernel::V3D> &directions,
                      const std::vector<double> &radii,
                      const Mantid::Kernel::V3D &originEllipsoid,
                      double zPlane) {
  // Translate into ellipsoid
  const double z = zPlane - originEllipsoid.Z();

  bool hasCut = false;
  // For each axis check if the z point is between the z values of the
  // axis endpoints
  int counter = 0;
  for (const auto &direction : directions) {
    const auto endpoint1 = direction[2] * radii[counter];
    const auto endpoint2 = -1 * direction[2] * radii[counter];

    if (isBetweenEndpoints(endpoint1, endpoint2, z)) {
      hasCut = true;
      break;
    }
    ++counter;
  }

  return hasCut;
}

/**
 * Gets the projections of the ellipsoid direcitons onto the xyz axes
 * @param directions: the ellipsoid direcitons
 * @param radii: the radii
 * @return the length of the largest radius projection per {x,y,z}
 */
std::vector<double>
getProjectionLengths(const std::vector<Mantid::Kernel::V3D> &directions,
                     std::vector<double> radii) {
  std::vector<Mantid::Kernel::V3D> directionsScaled;

  for (int index = 0; index < 3; ++index) {
    directionsScaled.emplace_back(directions[index] * radii[index]);
  }

  // We group the magnitudes of the x, y and z components
  std::vector<double> x = {std::abs(directionsScaled[0].X()),
                           std::abs(directionsScaled[1].X()),
                           std::abs(directionsScaled[2].X())};
  std::vector<double> y = {std::abs(directionsScaled[0].Y()),
                           std::abs(directionsScaled[1].Y()),
                           std::abs(directionsScaled[2].Y())};
  std::vector<double> z = {std::abs(directionsScaled[0].Z()),
                           std::abs(directionsScaled[1].Z()),
                           std::abs(directionsScaled[2].Z())};

  // Pick the largest element for each component
  auto xMax = std::max_element(std::begin(x), std::end(x));
  auto yMax = std::max_element(std::begin(y), std::end(y));
  auto zMax = std::max_element(std::begin(z), std::end(z));

  return std::vector<double>{*xMax, *yMax, *zMax};
}

MantidQt::SliceViewer::PeakBoundingBox getPeakBoundingBoxForEllipsoid(
    const std::vector<Mantid::Kernel::V3D> &directions,
    const std::vector<double> &radii,
    const Mantid::Kernel::V3D &originEllipsoid) {
  // Get the length of largest projection onto x,y,z
  auto projectionLengths = getProjectionLengths(directions, radii);

  using namespace MantidQt::SliceViewer;

  // Corners
  EllipsoidPlaneSliceCalculator calc;
  auto zoomOutFactor = calc.getZoomOutFactor();
  const double leftValue =
      originEllipsoid.X() - zoomOutFactor * projectionLengths[0];
  const double rightValue =
      originEllipsoid.X() + zoomOutFactor * projectionLengths[0];
  const double bottomValue =
      originEllipsoid.Y() - zoomOutFactor * projectionLengths[1];
  const double topValue =
      originEllipsoid.Y() + zoomOutFactor * projectionLengths[1];

  Left left(leftValue);
  Right right(rightValue);
  Bottom bottom(bottomValue);
  Top top(topValue);
  SlicePoint slicePoint(originEllipsoid.Z());

  return PeakBoundingBox(left, right, top, bottom, slicePoint);
}

double EllipsoidPlaneSliceCalculator::getZoomOutFactor() const {
  return m_zoomOutFactor;
}
} // namespace SliceViewer
} // namespace Mantid

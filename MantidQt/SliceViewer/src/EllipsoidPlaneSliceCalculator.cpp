#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include <cmath>

namespace
{
/**
 *  Calculates the first of two fractions for the origin
 * @param m: the ellipsoid matrix
 * @param sqrtFactor: a required factor
 * @param cosValue: a cosine value
 * @param sinValue: a sin value
 * @returns a partial fraction for the origin calculation
 */
double
calculatePartialFractionForOriginPart1(const Mantid::Kernel::Matrix<double> &m,
                                       double sqrtFactor, double cosValue,
                                       double sinValue)
{
    // Calculate the numerator
    const auto numerator
        = ((m[1][2] + m[2][1]) * cosValue - (m[0][2] + m[2][0]) * sinValue);

    // Calculate the denominator
    const auto m00_minus_m11_squared = pow(m[0][0] - m[1][1], 2);
    const auto m01_plus_m10_squared = pow(m[0][1] + m[1][0], 2);
    const auto denominator = m[0][0] * m[0][0] * sqrtFactor
                             - m[1][1] * m[1][1] * sqrtFactor
                             + m00_minus_m11_squared + m01_plus_m10_squared;

    return numerator / denominator;
}

/**
 *  Calculates the second of two fractions for the origin
 * @param m: the ellipsoid matrix
 * @param sqrtFactor: a required factor
 * @param cosValue: a cosine value
 * @param sinValue: a sin value
 * @returns a partial fraction for the origin calculation
 */
double
calculatePartialFractionForOriginPart2(const Mantid::Kernel::Matrix<double> &m,
                                       double sqrtFactor, double cosValue,
                                       double sinValue)
{
    // Calculate the numerator
    const auto numerator
        = ((m[0][2] + m[2][0]) * cosValue + (m[1][2] + m[2][1]) * sinValue);

    // Calculate the denominator
    const auto m00_minus_m11_squared = pow(m[0][0] - m[1][1], 2);
    const auto m01_plus_m10_squared = pow(m[0][1] + m[1][0], 2);
    const auto denominator = -m[0][0] * m[0][0] * sqrtFactor
                             + m[1][1] * m[1][1] * sqrtFactor
                             + m00_minus_m11_squared + m01_plus_m10_squared;

    return numerator / denominator;
}

double getNumeratorForRadiusCalculation(const Mantid::Kernel::Matrix<double> &m,
                                        double zPlane)
{
    const auto part1 = m[0][0] * pow(m[0][2] * zPlane + m[2][0] * zPlane, 2);
    const auto part2 = m[1][1] * pow(m[1][3] * zPlane + m[2][1] * zPlane, 2);
    const auto part3 = (m[2][2] * pow(zPlane, 2) - 1)
                       * pow(m[0][1] + m[1][0], 2);
    const auto part4 = (m[0][1] + m[1][0]) * (m[0][2] + m[2][0])
                       * (m[1][2] + m[2][1]) * pow(zPlane, 2);
    const auto part5 = -(m[2][2] * pow(zPlane, 2) - 1) * 4 * m[0][0] * m[1][1];

    return part1 + part2 + part3 + part4 + part5;
}
}

namespace Mantid
{
namespace SliceViewer
{

SliceEllipseInfo EllipsoidPlaneSliceCalculator::getSlicePlaneInfo(
    std::vector<Mantid::Kernel::V3D> directions,
    std::vector<Mantid::Kernel::V3D> radii, Mantid::Kernel::V3D originEllipsoid,
    double zPlane)
{
    // Setup the Ellipsoid Matrix
    Kernel::Matrix<double> m;
    m.setMem(3, 3);
    setupEllipsoidMatrix(m, directions, radii);

    // Clean the cache variables
    setupCache(m);

    // Get origin of slice ellipse
    const auto origin = getOriginOfSliceEllipse(m, zPlane);

    // Get the radii
    const auto ellipseRadii = getRadii(m, zPlane);

    // TODO Perform backtransformation

    return SliceEllipseInfo(origin, ellipseRadii[0], ellipseRadii[1], m_angle);
}

void EllipsoidPlaneSliceCalculator::setupEllipsoidMatrix(
    Kernel::Matrix<double> &m, std::vector<Mantid::Kernel::V3D> directions,
    std::vector<Mantid::Kernel::V3D> radii)
{
    auto vec0 = std::vector<double>(directions[0]);
    auto vec1 = std::vector<double>(directions[1]);
    auto vec2 = std::vector<double>(directions[2]);

    // TODO: reconstruct Matrix

    m.setColumn(0, vec0);
    m.setColumn(1, vec1);
    m.setColumn(2, vec2);
}

void EllipsoidPlaneSliceCalculator::setupCache(const Kernel::Matrix<double> &m)
{
    m_angle = 0.0;
    m_sinValue = 0.0;
    m_cosValue = 0.0;
    m_squareRootPrefactor = 0.0;

    // Set the prefactor which is used in several places
    setSquareRootPrefactor(m);
    m_angle = calculateAngle(m);
    setCosValue(m_angle);
    setSinValue(m_angle);
}

/**
  Calculates the angle that is need to rotate our canonical system into the
 eigen system of the ellipse
 * @param m12 : m12 parameter of the Ellipsoid matrix
 * @param m21 : m21 parameter of the Ellipsoid matrix
 * @param m11 : m11 parameter of the Ellipsoid matrix
 * @param m22 : m22 parameter of the Ellipsoid matrix
 * @return the angle required for the rotation
 */
double EllipsoidPlaneSliceCalculator::calculateAngle(
    const Kernel::Matrix<double> &m) const
{
    const auto ratio = (m[0][1] + m[1][0]) / (m[0][0] - m[1][1]);
    return std::atan(ratio) / 2.0;
}

void EllipsoidPlaneSliceCalculator::setSinValue(double angle)
{
    m_sinValue = std::sin(angle);
}

void EllipsoidPlaneSliceCalculator::setCosValue(double angle)
{
    m_cosValue = std::cos(angle);
}

Mantid::Kernel::V3D EllipsoidPlaneSliceCalculator::getOriginOfSliceEllipse(
    const Kernel::Matrix<double> &m, double zPlane)
{
    // General elements
    const auto prefactor = m_squareRootPrefactor * (m[0][0] - m[1][1]) * zPlane;
    const auto fractionPartial1 = calculatePartialFractionForOriginPart1(
        m, m_squareRootPrefactor, m_cosValue, m_sinValue);
    const auto fractionPartial2 = calculatePartialFractionForOriginPart1(
        m, m_squareRootPrefactor, m_cosValue, m_sinValue);

    // Calcuate the originX value
    const auto fraction1X = -m_cosValue * fractionPartial1;
    const auto fraction2X = m_sinValue * fractionPartial2;
    const auto originX = prefactor * (fraction1X + fraction2X);

    // Calcuate the originY value
    const auto fraction1Y = m_sinValue * fractionPartial1;
    const auto fraction2Y = m_cosValue * fractionPartial2;
    const auto originY = prefactor * (fraction1Y + fraction2Y);

    return Mantid::Kernel::V3D(originX, originY, zPlane);
}

Mantid::Kernel::V2D
EllipsoidPlaneSliceCalculator::getRadii(const Kernel::Matrix<double> &m,
                                        double zPlane)
{
    // Get general parts
    const auto numerator = getNumeratorForRadiusCalculation(m, zPlane);
    const auto denominatorPart1 = -pow(m[0][1] + m[1][0], 2)
                                  + 4 * m[0][0] * m[1][1];

    // Get the first radius
    const auto denominatorPart2Radius1 = m[0][0] * (1 + m_squareRootPrefactor)
                                         + m[1][1]
                                               * (1 - m_squareRootPrefactor);
    const auto denominatorRadius1 = denominatorPart1 * denominatorPart2Radius1;
    const auto radius1 = std::sqrt(2.0 * numerator / denominatorRadius1);

    // Get the second radius
    const auto denominatorPart2Radius2 = m[0][0] * (1 - m_squareRootPrefactor)
                                         + m[1][1]
                                               * (1 + m_squareRootPrefactor);
    const auto denominatorRadius2 = denominatorPart1 * denominatorPart2Radius2;
    const auto radius2 = std::sqrt(2.0 * numerator / denominatorRadius2);

    return Kernel::V2D(radius1, radius2);
}

double EllipsoidPlaneSliceCalculator::getAngle() { return m_angle; }

/**
 Sets the square root pre factor
 * @param m12 : m12 parameter of the Ellipsoid matrix
 * @param m21 : m21 parameter of the Ellipsoid matrix
 * @param m11 : m11 parameter of the Ellipsoid matrix
 * @param m22 : m22 parameter of the Ellipsoid matrix
 *
 * preFactor = sqrt(1 - (m12 -m21)^2 / (m11 - m22)^2 )
 *
 */
void EllipsoidPlaneSliceCalculator::setSquareRootPrefactor(
    const Kernel::Matrix<double> &m)
{
    const auto valueInSquareRoot
        = 1 + pow(m[0][1] + m[1][0], 2) / pow(m[0][0] - m[1][1], 2);
    m_squareRootPrefactor = sqrt(valueInSquareRoot);
}
}
}

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <algorithm>
#include <boost/version.hpp>
#include <sstream>

#if BOOST_VERSION < 106700
#include <boost/math/common_factor.hpp>
using boost::math::gcd;
#else
#include <boost/integer/common_factor.hpp>
using boost::integer::gcd;
#endif

namespace {
/** transform vector into form, used to describe directions in crystallogaphical
 * coodinate system, assuming that the vector describes perpendicular to a
 * crystallogaphic plain or is close to such plain.
 *
 * As crystallographical coordinate sytem is based on 3 integers, eps is used
 * as accuracy to convert into integers
 */
double nearInt(double val, double eps, double mult) noexcept {
  if (val > 0.0) {
    if (val < 1.0) {
      mult /= val;
    } else {
      if (std::abs(val - std::round(val)) > eps) {
        mult *= std::ceil(val / eps) * eps / val;
      }
    }
  }
  return mult;
}
} // namespace

namespace Mantid::Kernel {

/**
  Sets the vector position based on spherical coordinates

  @param R :: The R value (distance)
  @param theta :: The theta value (in degrees) = the polar angle away from the
  +Z axis.
  @param phi :: The phi value (in degrees) = the azimuthal angle, where 0 points
  along +X and rotates clockwise in the XY plane
*/
void V3D::spherical(const double R, const double theta, const double phi) noexcept {
  constexpr double deg2rad = M_PI / 180.0;
  spherical_rad(R, theta * deg2rad, phi * deg2rad);
}

/**
  Sets the vector position based on spherical coordinates, in radians

  @param R :: The R value (distance)
  @param polar :: the polar angle (in radians) away from the +Z axis.
  @param azimuth :: the azimuthal angle (in radians), where 0 points along +X
  and rotates clockwise in the XY plane
*/
void V3D::spherical_rad(const double R, const double polar, const double azimuth) noexcept {
  m_pt[2] = R * cos(polar);
  const double ct = R * sin(polar);
  m_pt[0] = ct * cos(azimuth);
  m_pt[1] = ct * sin(azimuth);

  // Setting this way can lead to very small values of x & y that should really
  // be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(m_pt[0]) < Tolerance)
    m_pt[0] = 0.0;
  if (std::abs(m_pt[1]) < Tolerance)
    m_pt[1] = 0.0;
}

/**
  Sets the vector position based on azimuth and polar angle, in RADIANS, in the
  SNS instrument coordinate system,
    where +Z = beam direction, +Y = vertical.

  @param R :: The R value (distance)
  @param azimuth :: The azimuthal angle (in Radians)
  @param polar :: The polar value (in Radians)
*/

void V3D::azimuth_polar_SNS(const double R, const double azimuth, const double polar) noexcept {
  m_pt[1] = R * cos(polar);
  const double ct = R * sin(polar);
  m_pt[0] = ct * cos(azimuth);
  m_pt[2] = ct * sin(azimuth);

  // Setting this way can lead to very small values of x & y that should really
  // be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(m_pt[0]) < Tolerance)
    m_pt[0] = 0.0;
  if (std::abs(m_pt[1]) < Tolerance)
    m_pt[1] = 0.0;
  if (std::abs(m_pt[2]) < Tolerance)
    m_pt[2] = 0.0;
}

/** Return the vector's position in spherical coordinates
 *  @param R ::     Returns the radial distance
 *  @param theta :: Returns the theta angle in degrees
 *  @param phi ::   Returns the phi (azimuthal) angle in degrees
 */
void V3D::getSpherical(double &R, double &theta, double &phi) const noexcept {
  constexpr double rad2deg = 180.0 / M_PI;
  R = norm();
  theta = 0.0;
  if (R != 0.0)
    theta = acos(m_pt[2] / R) * rad2deg;
  phi = atan2(m_pt[1], m_pt[0]) * rad2deg;
}

/**
  Normalises the vector and returns its original length
  @return the norm of the vector before normalization
*/
double V3D::normalize() {
  const double ND(norm());
  if (ND == 0) {
    throw std::runtime_error("Unable to normalize a zero length vector.");
  }
  this->operator/=(ND);
  return ND;
}

/** Round each component to the nearest integer */
void V3D::round() noexcept {
  std::transform(m_pt.cbegin(), m_pt.cend(), m_pt.begin(), [](auto p) { return std::round(p); });
}

/** Calculates the zenith angle (theta) of this vector with respect to another
 *  @param v :: The other vector
 *  @return The azimuthal angle in radians (0 < theta < pi)
 */
double V3D::zenith(const V3D &v) const noexcept {
  const double R = distance(v);
  if (R != 0.0) {
    const double zOffset = m_pt[2] - v.m_pt[2];
    return acos(zOffset / R);
  } else {
    return 0.0;
  }
}

/** Calculates the angle between this and another vector.
 *
 *  @param v :: The other vector
 *  @return The angle between the vectors in radians (0 < theta < pi)
 */
double V3D::angle(const V3D &v) const {
  const double ratio = this->cosAngle(v);
  if (ratio >= 1.0)       // NOTE: Due to rounding errors, if v is
    return 0.0;           //       is nearly the same as "this" or
  else if (ratio <= -1.0) //       as "-this", ratio can be slightly
    return M_PI;          //       more than 1 in absolute value.
                          //       That causes acos() to return NaN.
  return acos(ratio);
}

/** Calculates the cosine of angle between this and another vector.
 *
 *  @param v :: The other vector
 *  @return cosine of the angle between the vectors
 */
double V3D::cosAngle(const V3D &v) const {
  const double n1 = norm();
  const double n2 = v.norm();
  if (n1 == 0. || n2 == 0.) {
    throw std::runtime_error("Cannot calculate an angle when one of the vectors has zero length.");
  }
  return this->scalar_prod(v) / (n1 * n2);
}

/**
   Re-express this point as components of A,B,C.
   Assuming that A,B,C form a basis set (which does not have to be
   orthonormal).
   @param A :: Unit vector in basis
   @param B :: Unit vector in basis
   @param C :: Unit vector in basis
   @retval -1 :: The points do not form a basis set.
   @retval 0  :: Vec3D has successfully been re-expressed.
*/
int V3D::reBase(const V3D &A, const V3D &B, const V3D &C) noexcept {
  Matrix<double> T(3, 3);
  for (int i = 0; i < 3; i++) {
    T[i][0] = A[i];
    T[i][1] = B[i];
    T[i][2] = C[i];
  }
  const double det = T.Invert();
  if (std::abs(det) < 1e-13) // failed
    return -1;
  rotate(T);
  return 0;
}

/**
  Rotate a point by a matrix
  @param A :: Rotation matrix (needs to be >= 3x3)
*/
void V3D::rotate(const Kernel::Matrix<double> &A) noexcept {
  const double xold(m_pt[0]), yold(m_pt[1]), zold(m_pt[2]);
  for (size_t i = 0; i < m_pt.size(); ++i) {
    m_pt[i] = A[i][0] * xold + A[i][1] * yold + A[i][2] * zold;
  }
}

/**
  Determines if this,B,C are colinear
  @param Bv :: Vector to test
  @param Cv :: Vector to test
  @return false if no colinear and true if they are (within Tolerance)
*/
bool V3D::coLinear(const V3D &Bv, const V3D &Cv) const noexcept {
  const V3D &Av = *this;
  const V3D Tmp((Bv - Av).cross_prod(Cv - Av));
  return Tmp.norm() <= Tolerance;
}

/**
  Checks the size of the vector
  @param tolerance :: size of the biggest zero vector allowed.
  @return true if the vector's elements are less in magnitude than tolerance
*/
bool V3D::nullVector(const double tolerance) const noexcept {
  return std::none_of(m_pt.cbegin(), m_pt.cend(), [&tolerance](const auto p) { return std::abs(p) > tolerance; });
}

bool V3D::unitVector(const double tolerance) const noexcept {
  // NOTE could be made more efficient using norm2()
  const auto l = norm();
  return std::abs(l - 1.) < tolerance;
}

/**
   Calculates the index of the primary direction (if there is one)
   @param tolerance :: Tolerance accepted
   @retval range -3,-2,-1 1,2,3  if the vector
   is orientaged within tolerance on the x,y,z direction (the sign
   indecates the direction to the +ve side )
   @retval 0 :: No master direction
*/
int V3D::masterDir(const double tolerance) const noexcept {
  // Calc max dist
  double max = m_pt[0] * m_pt[0];
  double other = max;
  double u2 = m_pt[1] * m_pt[1];
  int idx = (m_pt[0] > 0) ? 1 : -1;
  if (u2 > max) {
    max = u2;
    idx = (m_pt[1] > 0) ? 2 : -2;
  }
  other += u2;
  u2 = m_pt[2] * m_pt[2];
  if (u2 > max) {
    max = u2;
    idx = (m_pt[2] > 0) ? 3 : -3;
  }
  other += u2;
  other -= max;
  if ((other / max) > tolerance) // doesn't have master direction
  {
    return 0;
  }
  return idx;
}

/** Take a list of 2 vectors and makes a 3D orthogonal system out of them
 * The first vector i0 is taken as such.
 * The second vector is made perpendicular to i0, in the plane of i0-i1
 * The third vector is made perpendicular to the plane i0-i1 by performing the
 *cross product of 0 and 1
 *
 * @param vectors :: list of 2 vectors
 * @return list of 3 vectors
 */
std::vector<V3D> V3D::makeVectorsOrthogonal(const std::vector<V3D> &vectors) {
  if (vectors.size() != 2)
    throw std::invalid_argument("makeVectorsOrthogonal() only works with 2 vectors");

  const V3D v0 = Kernel::normalize(vectors[0]);
  V3D v1 = Kernel::normalize(vectors[1]);

  std::vector<V3D> out;
  out.reserve(3);
  out.emplace_back(v0);

  // Make a rotation 90 degrees from 0 to 1
  Quat q(v0, v1);
  q.setRotation(90);
  // Rotate v1 so it is 90 deg
  v1 = v0;
  q.rotate(v1);
  out.emplace_back(v1);

  // Finally, the 3rd vector = cross product of 0 and 1
  V3D v2 = v0.cross_prod(v1);
  out.emplace_back(v2);
  return out;
}

/**
  Read data from a stream.
  \todo Check Error handling
  @param IX :: Input Stream
*/
void V3D::read(std::istream &IX) { IX >> m_pt[0] >> m_pt[1] >> m_pt[2]; }

void V3D::write(std::ostream &OX) const
/**
  Write out the point values
  @param OX :: Output stream
*/
{
  OX << m_pt[0] << " " << m_pt[1] << " " << m_pt[2];
}

/** @return the vector as a string "X Y Z" */
std::string V3D::toString() const {
  std::ostringstream mess;
  this->write(mess);
  return mess.str();
}

/** Sets the vector using a string
 * @param str :: the vector as a string "X Y Z" */
void V3D::fromString(const std::string &str) {
  std::istringstream mess(str);
  this->read(mess);
}

/**
  Prints a text representation of itself in format "[x,y,z]"
  @param os :: the Stream to output to
*/
void V3D::printSelf(std::ostream &os) const { os << "[" << m_pt[0] << "," << m_pt[1] << "," << m_pt[2] << "]"; }

/**
  Read data from a stream in the format returned by printSelf ("[x,y,z]").
  @param IX :: Input Stream
  @throw std::runtime_error if the input is of wrong format
*/
void V3D::readPrinted(std::istream &IX) {
  std::string in;
  std::getline(IX, in);
  size_t i = in.find_first_of('[');
  if (i == std::string::npos)
    throw std::runtime_error("Wrong format for V3D input: " + in);
  size_t j = in.find_last_of(']');
  if (j == std::string::npos || j < i + 6)
    throw std::runtime_error("Wrong format for V3D input: " + in);

  size_t c1 = in.find_first_of(',');
  size_t c2 = in.find_first_of(',', c1 + 1);
  if (c1 == std::string::npos || c2 == std::string::npos)
    throw std::runtime_error("Wrong format for V3D input: [" + in + "]");

  m_pt[0] = std::stod(in.substr(i + 1, c1 - i - 1));
  m_pt[1] = std::stod(in.substr(c1 + 1, c2 - c1 - 1));
  m_pt[2] = std::stod(in.substr(c2 + 1, j - c2 - 1));
}

/**
  Prints a text representation of itself
  @param os :: the Stream to output to
  @param v :: the vector to output
  @return the output stream
  */
std::ostream &operator<<(std::ostream &os, const V3D &v) {
  v.printSelf(os);
  return os;
}

std::istream &operator>>(std::istream &IX, V3D &A)
/**
  Calls Vec3D method write to output class
  @param IX :: Input Stream
  @param A :: Vec3D to write
  @return Current state of stream
*/
{
  A.readPrinted(IX);
  return IX;
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param name :: name of the data to create
 */
void V3D::saveNexus(::NeXus::File *file, const std::string &name) const {
  file->makeData(name, ::NeXus::FLOAT64, 3, true);
  file->putData(m_pt.data());
  file->closeData();
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param name :: name of the data to open
 */
void V3D::loadNexus(::NeXus::File *file, const std::string &name) {
  std::vector<double> data;
  file->readData(name, data);
  if (data.size() != 3)
    throw std::runtime_error("Unexpected data size when reading a V3D NXS field '" + name + "'. Expected 3.");
  std::copy(data.cbegin(), data.cend(), m_pt.begin());
}

double V3D::toMillerIndexes(double eps) {
  if (eps < 0)
    eps = -eps;
  if (eps < FLT_EPSILON)
    eps = FLT_EPSILON;

  // assuming eps is in 1.e-x form

  double ax = std::abs(m_pt[0]);
  double ay = std::abs(m_pt[1]);
  double az = std::abs(m_pt[2]);

  double amax = (ax > ay) ? ax : ay;
  amax = (az > amax) ? az : amax;
  if (amax < FLT_EPSILON)
    throw(std::invalid_argument("vector length is less then accuracy requested"));

  if (ax < eps) {
    m_pt[0] = 0;
    ax = 0;
  }
  if (ay < eps) {
    m_pt[1] = 0;
    ay = 0;
  }
  if (az < eps) {
    m_pt[2] = 0;
    az = 0;
  }

  double mult(1);
  mult = nearInt(ax, eps, mult);
  mult = nearInt(ay, eps, mult);
  mult = nearInt(az, eps, mult);

  const size_t iax = std::lround(ax * mult / eps);
  const size_t iay = std::lround(ay * mult / eps);
  const size_t iaz = std::lround(az * mult / eps);

  const size_t div = gcd(iax, gcd(iay, iaz));
  mult /= (static_cast<double>(div) * eps);
  this->operator*=(mult);

  return mult;
}

/**
   Comparator function for sorting list of 3D vectors based on their magnitude.
   @param v1  first vector
   @param v2  seconde vector
   @return true if v1.norm() < v2.norm().
 */
bool V3D::compareMagnitude(const V3D &v1, const V3D &v2) {
  const double mag_sq_1 = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2];
  const double mag_sq_2 = v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2];
  return (mag_sq_1 < mag_sq_2);
}

/**
 * Get direction angles from direction cosines.
 * @param inDegrees : optional argument for specifying in radians (false).
 * Defaults to true.
 * @return V3D containing anlges.
 */
V3D V3D::directionAngles(bool inDegrees) const {
  const double conversionFactor = inDegrees ? 180. / M_PI : 1.0;
  const double divisor = this->norm();
  if (divisor == 0.) {
    throw std::runtime_error("Cannot calculate direction angles for zero length vector");
  }
  return V3D(conversionFactor * acos(m_pt[0] / divisor), conversionFactor * acos(m_pt[1] / divisor),
             conversionFactor * acos(m_pt[2] / divisor));
}

/**
  Vector maximum absolute integer value
  @return maxCoeff()
*/
int V3D::maxCoeff() {
  int MaxOrder = 0;
  if (std::abs(static_cast<int>(m_pt[0])) > MaxOrder)
    MaxOrder = std::abs(static_cast<int>(m_pt[0]));
  if (std::abs(static_cast<int>(m_pt[1])) > MaxOrder)
    MaxOrder = std::abs(static_cast<int>(m_pt[1]));
  if (std::abs(static_cast<int>(m_pt[2])) > MaxOrder)
    MaxOrder = std::abs(static_cast<int>(m_pt[2]));
  return MaxOrder;
}

/**
  Calculates the absolute value.
  @return The absolute value
*/
V3D V3D::absoluteValue() const { return V3D(std::abs(m_pt[0]), std::abs(m_pt[1]), std::abs(m_pt[2])); }

/**
  Calculates the error of the HKL to compare with tolerance
  @return The error
*/
double V3D::hklError() const {
  return std::abs(m_pt[0] - std::round(m_pt[0])) + //
         std::abs(m_pt[1] - std::round(m_pt[1])) + //
         std::abs(m_pt[2] - std::round(m_pt[2]));
}

} // namespace Mantid::Kernel

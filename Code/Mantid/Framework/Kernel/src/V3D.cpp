#include <iostream>
#include <cmath>
#include <float.h>
#include <vector>

#include "MantidKernel/V3D.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Quat.h"
#include <boost/math/common_factor.hpp>

namespace Mantid {
namespace Kernel {

/// Constructor [Null]
V3D::V3D() : x(0), y(0), z(0) {}

/// Value constructor
V3D::V3D(const double xx, const double yy, const double zz)
    : x(xx), y(yy), z(zz) {}

/// Copy constructor
V3D::V3D(const V3D &v) : x(v.x), y(v.y), z(v.z) {}

/**
  Sets the vector position based on spherical coordinates

  @param R :: The R value (distance)
  @param theta :: The theta value (in degrees) = the polar angle away from the
  +Z axis.
  @param phi :: The phi value (in degrees) = the azimuthal angle, where 0 points
  along +X and rotates counter-clockwise in the XY plane
*/
void V3D::spherical(const double &R, const double &theta, const double &phi) {
  const double deg2rad = M_PI / 180.0;
  z = R * cos(theta * deg2rad);
  const double ct = sin(theta * deg2rad);
  x = R * ct * cos(phi * deg2rad);
  y = R * ct * sin(phi * deg2rad);

  // Setting this way can lead to very small values of x & y that should really
  // be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(x) < Tolerance)
    x = 0.0;
  if (std::abs(y) < Tolerance)
    y = 0.0;
}

/**
  Sets the vector position based on spherical coordinates, in radians

  @param R :: The R value (distance)
  @param polar :: the polar angle (in radians) away from the +Z axis.
  @param azimuth :: the azimuthal angle (in radians), where 0 points along +X
  and rotates counter-clockwise in the XY plane
*/
void V3D::spherical_rad(const double &R, const double &polar,
                        const double &azimuth) {
  z = R * cos(polar);
  const double ct = R * sin(polar);
  x = ct * cos(azimuth);
  y = ct * sin(azimuth);

  // Setting this way can lead to very small values of x & y that should really
  // be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(x) < Tolerance)
    x = 0.0;
  if (std::abs(y) < Tolerance)
    y = 0.0;
}

/**
  Sets the vector position based on azimuth and polar angle, in RADIANS, in the
  SNS instrument coordinate system,
    where +Z = beam direction, +Y = vertical.

  @param R :: The R value (distance)
  @param azimuth :: The azimuthal angle (in Radians)
  @param polar :: The polar value (in Radians)
*/

void V3D::azimuth_polar_SNS(const double &R, const double &azimuth,
                            const double &polar) {
  y = R * cos(polar);
  const double ct = R * sin(polar);
  x = ct * cos(azimuth);
  z = ct * sin(azimuth);

  // Setting this way can lead to very small values of x & y that should really
  // be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(x) < Tolerance)
    x = 0.0;
  if (std::abs(y) < Tolerance)
    y = 0.0;
  if (std::abs(z) < Tolerance)
    z = 0.0;
}

/**
  Assignment operator
  @param rhs :: V3D to copy
  @return *this
*/
V3D &V3D::operator=(const V3D &rhs) {
  x = rhs.x;
  y = rhs.y;
  z = rhs.z;
  return *this;
}

/// Destructor
V3D::~V3D() {}

/**
  Addtion operator
   @param v :: Vector to add
   @return *this+v;
*/
V3D V3D::operator+(const V3D &v) const {
  V3D out(*this);
  out += v;
  return out;
}

/**
  Subtraction operator
  @param v :: Vector to sub.
  @return *this-v;
*/
V3D V3D::operator-(const V3D &v) const {
  V3D out(*this);
  out -= v;
  return out;
}

/**
  Inner product
  @param v :: Vector to sub.
  @return *this * v;
*/
V3D V3D::operator*(const V3D &v) const {
  V3D out(*this);
  out *= v;
  return out;
}

/**
  Inner division
  @param v :: Vector to divide
  @return *this * v;
*/
V3D V3D::operator/(const V3D &v) const {
  V3D out(*this);
  out /= v;
  return out;
}

/**
  Self-Addition operator
  @param v :: Vector to add.
  @return *this+=v;
*/
V3D &V3D::operator+=(const V3D &v) {
  x += v.x;
  y += v.y;
  z += v.z;
  return *this;
}

/**
  Self-Subtraction operator
  @param v :: Vector to sub.
  @return *this-v;
*/
V3D &V3D::operator-=(const V3D &v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
  return *this;
}

/**
  Self-Inner product
  @param v :: Vector to multiply
  @return *this*=v;
*/
V3D &V3D::operator*=(const V3D &v) {
  x *= v.x;
  y *= v.y;
  z *= v.z;
  return *this;
}

/**
  Self-Inner division
  @param v :: Vector to divide
  @return *this*=v;
*/
V3D &V3D::operator/=(const V3D &v) {
  x /= v.x;
  y /= v.y;
  z /= v.z;
  return *this;
}

/**
  Scalar product
  @param D :: value to scale
  @return this * D
 */
V3D V3D::operator*(const double D) const {
  V3D out(*this);
  out *= D;
  return out;
}

/**
  Scalar divsion
  @param D :: value to scale
  @return this / D
*/
V3D V3D::operator/(const double D) const {
  V3D out(*this);
  out /= D;
  return out;
}

/**
  Scalar product
  @param D :: value to scale
  @return this *= D
*/
V3D &V3D::operator*=(const double D) {
  x *= D;
  y *= D;
  z *= D;
  return *this;
}

/**
  Scalar division
  @param D :: value to scale
  @return this /= D
  \todo ADD TOLERANCE
*/
V3D &V3D::operator/=(const double D) {
  if (D != 0.0) {
    x /= D;
    y /= D;
    z /= D;
  }
  return *this;
}

/**
  Equals operator with tolerance factor
  @param v :: V3D for comparison
  @return true if the items are equal
*/
bool V3D::operator==(const V3D &v) const {
  using namespace std;
  return !(fabs(x - v.x) > Tolerance || fabs(y - v.y) > Tolerance ||
           fabs(z - v.z) > Tolerance);
}

/** Not equals operator with tolerance factor.
 *  @param other :: The V3D to compare against
 *  @returns True if the vectors are different
 */
bool V3D::operator!=(const V3D &other) const {
  return !(this->operator==(other));
}

/**
  compare
  @return true if V is greater
 */
bool V3D::operator<(const V3D &V) const {
  if (x != V.x)
    return x < V.x;
  if (y != V.y)
    return y < V.y;
  return z < V.z;
}

/// Comparison operator greater than.
bool V3D::operator>(const V3D &rhs) const
{
    return rhs < *this;
}

/**
  Sets the vector position from a triplet of doubles x,y,z
  @param xx :: The X coordinate
  @param yy :: The Y coordinate
  @param zz :: The Z coordinate
*/
void V3D::operator()(const double xx, const double yy, const double zz) {
  x = xx;
  y = yy;
  z = zz;
  return;
}

/**
  Set is x position
  @param xx :: The X coordinate
*/
void V3D::setX(const double xx) { x = xx; }

/**
  Set is y position
  @param yy :: The Y coordinate
*/
void V3D::setY(const double yy) { y = yy; }

/**
  Set is z position
  @param zz :: The Z coordinate
*/
void V3D::setZ(const double zz) { z = zz; }

/**
  Returns the axis value based in the index provided
  @param Index :: 0=x, 1=y, 2=z
  @return a double value of the requested axis
*/
const double &V3D::operator[](const size_t Index) const {
  switch (Index) {
  case 0:
    return x;
  case 1:
    return y;
  case 2:
    return z;
  default:
    throw Kernel::Exception::IndexError(Index, 2,
                                        "V3D::operator[] range error");
  }
}

/**
  Returns the axis value based in the index provided
  @param Index :: 0=x, 1=y, 2=z
  @return a double value of the requested axis
*/
double &V3D::operator[](const size_t Index) {
  switch (Index) {
  case 0:
    return x;
  case 1:
    return y;
  case 2:
    return z;
  default:
    throw Kernel::Exception::IndexError(Index, 2,
                                        "V3D::operator[] range error");
  }
}

/** Return the vector's position in spherical coordinates
 *  @param R ::     Returns the radial distance
 *  @param theta :: Returns the theta angle in degrees
 *  @param phi ::   Returns the phi (azimuthal) angle in degrees
 */
void V3D::getSpherical(double &R, double &theta, double &phi) const {
  const double rad2deg = 180.0 / M_PI;
  R = norm();
  theta = 0.0;
  if (R != 0.0)
    theta = acos(z / R) * rad2deg;
  phi = atan2(y, x) * rad2deg;
  return;
}

/**
  Vector length
  @return vec.length()
*/
double V3D::norm() const { return sqrt(x * x + y * y + z * z); }

/**
  Vector length without the sqrt
  @return vec.length()
*/
double V3D::norm2() const { return (x * x + y * y + z * z); }

/**
  Normalises the vector and
  then returns the scalar value of the vector
  @return Norm
*/
double V3D::normalize() {
  const double ND(norm());
  this->operator/=(ND);
  return ND;
}

/** Round each component to the nearest integer */
void V3D::round() {
  x = double(long(x + (x < 0 ? -0.5 : +0.5)));
  y = double(long(y + (y < 0 ? -0.5 : +0.5)));
  z = double(long(z + (z < 0 ? -0.5 : +0.5)));
}

/**
  Calculates the scalar product
  @param V :: The second vector to include in the calculation
  @return The scalar product of the two vectors
*/
double V3D::scalar_prod(const V3D &V) const {
  return (x * V.x + y * V.y + z * V.z);
}

/**
  Calculates the cross product. Returns (this * v).
  @param v :: The second vector to include in the calculation
  @return The cross product of the two vectors (this * v)
*/
V3D V3D::cross_prod(const V3D &v) const {
  return V3D(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

/**
  Calculates the distance between two vectors
  @param v :: The second vector to include in the calculation
  @return The distance between the two vectors
*/
double V3D::distance(const V3D &v) const {
  const double dx(x - v.x), dy(y - v.y), dz(z - v.z);
  return sqrt(dx * dx + dy * dy + dz * dz);
}

/** Calculates the zenith angle (theta) of this vector with respect to another
 *  @param v :: The other vector
 *  @return The azimuthal angle in radians (0 < theta < pi)
 */
double V3D::zenith(const V3D &v) const {
  double R = distance(v);
  double zOffset = z - v.z;
  if (R != 0.0) {
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
  double ratio = this->scalar_prod(v) / (this->norm() * v.norm());

  if (ratio >= 1.0)       // NOTE: Due to rounding errors, if v is
    return 0.0;           //       is nearly the same as "this" or
  else if (ratio <= -1.0) //       as "-this", ratio can be slightly
    return M_PI;          //       more than 1 in absolute value.
                          //       That causes acos() to return NaN.
  return acos(ratio);
}

int V3D::reBase(const V3D &A, const V3D &B, const V3D &C)
/**
   Re-express this point components of A,B,C.
   Assuming that A,B,C are form an basis set (which
   does not have to be othonormal.
   @param A :: Unit vector in basis
   @param B :: Unit vector in basis
   @param C :: Unit vector in basis
   @retval -1 :: The points do not form a basis set.
   @retval 0  :: Vec3D has successfully been re-expressed.
*/
{
  Matrix<double> T(3, 3);
  for (int i = 0; i < 3; i++) {
    T[i][0] = A[i];
    T[i][1] = B[i];
    T[i][2] = C[i];
  }
  const double det = T.Invert();
  if (fabs(det) < 1e-13) // failed
    return -1;
  rotate(T);
  return 0;
}

void V3D::rotate(const Kernel::Matrix<double> &A)
/**
  Rotate a point by a matrix
  @param A :: Rotation matrix (needs to be >3x3)
*/
{
  double xold(x), yold(y), zold(z);
  x = A[0][0] * xold + A[0][1] * yold + A[0][2] * zold;
  y = A[1][0] * xold + A[1][1] * yold + A[1][2] * zold;
  z = A[2][0] * xold + A[2][1] * yold + A[2][2] * zold;
}

/**
  Determines if this,B,C are collinear
  @param Bv :: Vector to test
  @param Cv :: Vector to test
  @return false is no colinear and true if they are (within Ptolerance)
*/
bool V3D::coLinear(const V3D &Bv, const V3D &Cv) const {
  const V3D &Av = *this;
  const V3D Tmp((Bv - Av).cross_prod(Cv - Av));
  return (Tmp.norm() > Tolerance) ? false : true;
}

bool V3D::nullVector(const double Tol) const
/**
  Checks the size of the vector
  @param Tol :: size of the biggest zero vector allowed.
  @retval 1 : the vector squared components
  magnitude are less than Tol
  @retval 0 :: Vector bigger than Tol
*/
{
  using namespace std;
  if (fabs(x) > Tol)
    return false;
  if (fabs(y) > Tol)
    return false;
  if (fabs(z) > Tol)
    return false;

  // Getting to this point means a null vector
  return true;
}

int V3D::masterDir(const double Tol) const
/**
   Calculates the index of the primary direction (if there is one)
   @param Tol :: Tolerance accepted
   @retval range -3,-2,-1 1,2,3  if the vector
   is orientaged within Tol on the x,y,z direction (the sign
   indecates the direction to the +ve side )
   @retval 0 :: No master direction
*/
{
  // Calc max dist
  double max = x * x;
  double other = max;
  double u2 = y * y;
  int idx = (x > 0) ? 1 : -1;
  if (u2 > max) {
    max = u2;
    idx = (y > 0) ? 2 : -2;
  }
  other += u2;
  u2 = z * z;
  if (u2 > max) {
    max = u2;
    idx = (z > 0) ? 3 : -3;
  }
  other += u2;
  other -= max;
  if ((other / max) > Tol) // doesn't have master direction
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
std::vector<V3D> V3D::makeVectorsOrthogonal(std::vector<V3D> &vectors) {
  if (vectors.size() != 2)
    throw std::invalid_argument(
        "makeVectorsOrthogonal() only works with 2 vectors");

  V3D v0 = vectors[0];
  v0.normalize();
  V3D v1 = vectors[1];
  v1.normalize();

  std::vector<V3D> out;
  out.push_back(v0);

  // Make a rotation 90 degrees from 0 to 1
  Quat q(v0, v1);
  q.setRotation(90);
  // Rotate v1 so it is 90 deg
  v1 = v0;
  q.rotate(v1);
  out.push_back(v1);

  // Finally, the 3rd vector = cross product of 0 and 1
  V3D v2 = v0.cross_prod(v1);
  out.push_back(v2);
  return out;
}

/**
  Read data from a stream.
  \todo Check Error handling
  @param IX :: Input Stream
*/
void V3D::read(std::istream &IX) {
  IX >> x >> y >> z;
  return;
}

void V3D::write(std::ostream &OX) const
/**
  Write out the point values
  @param OX :: Output stream
*/
{
  OX << x << " " << y << " " << z;
  return;
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
void V3D::printSelf(std::ostream &os) const {
  os << "[" << x << "," << y << "," << z << "]";
  return;
}

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

  x = atof(in.substr(i + 1, c1 - i - 1).c_str());
  y = atof(in.substr(c1 + 1, c2 - c1 - 1).c_str());
  z = atof(in.substr(c2 + 1, j - c2 - 1).c_str());

  return;
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
  double data[3] = {x, y, z};
  file->putData(data);
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
    throw std::runtime_error(
        "Unexpected data size when reading a V3D NXS field '" + name +
        "'. Expected 3.");
  x = data[0];
  y = data[1];
  z = data[2];
}

/** transform vector into form, used to describe directions in crystallogaphical
  *coodinate system, assuming that
  * the vector describes perpendicular to a crystallogaphic plain or is close to
  *such plain.
  *
  *  As crystallographical coordinate sytem is based on 3 integers, eps is used
  *as accuracy to convert into integers
*/
#define DINT(x) std::fabs((x) - double(size_t((x)+0.5)))
double nearInt(double val, double eps, double mult) {
  if (val > 0) {
    if (val < 1) {
      mult /= val;
    } else {
      if (DINT(val) > eps) {
        mult *= (double(size_t(val / eps) + 1) * eps / val);
      }
    }
  }
  return mult;
}
double V3D::toMillerIndexes(double eps) {
  if (eps < 0)
    eps = -eps;
  if (eps < FLT_EPSILON)
    eps = FLT_EPSILON;

  // assuming eps is in 1.e-x form

  double ax = std::fabs(x);
  double ay = std::fabs(y);
  double az = std::fabs(z);

  double amax = (ax > ay) ? ax : ay;
  amax = (az > amax) ? az : amax;
  if (amax < FLT_EPSILON)
    throw(
        std::invalid_argument("vector length is less then accuracy requested"));

  if (ax < eps) {
    x = 0;
    ax = 0;
  }
  if (ay < eps) {
    y = 0;
    ay = 0;
  }
  if (az < eps) {
    z = 0;
    az = 0;
  }

  double mult(1);
  mult = nearInt(ax, eps, mult);
  mult = nearInt(ay, eps, mult);
  mult = nearInt(az, eps, mult);

  size_t iax = size_t(ax * mult / eps + 0.5);
  size_t iay = size_t(ay * mult / eps + 0.5);
  size_t iaz = size_t(az * mult / eps + 0.5);

  size_t div = boost::math::gcd(iax, boost::math::gcd(iay, iaz));
  mult /= (double(div) * eps);
  x *= mult;
  y *= mult;
  z *= mult;

  return mult;
}

/**
   Comparator function for sorting list of 3D vectors based on their magnitude.
   @param v1  first vector
   @param v2  seconde vector
   @return true if v1.norm() < v2.norm().
 */
bool V3D::CompareMagnitude(const V3D &v1, const V3D &v2) {
  double mag_sq_1 = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2];
  double mag_sq_2 = v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2];
  return (mag_sq_1 < mag_sq_2);
}

/**
 * Get direction angles from direction cosines.
 * @param inDegrees : optional argument for specifying in radians (false).
 * Defaults to true.
 * @return V3D containing anlges.
 */
V3D V3D::directionAngles(bool inDegrees) const {
  double conversionFactor = 1.0;
  if (inDegrees) {
    conversionFactor = 180.0 / M_PI;
  }
  const double divisor = this->norm();
  return V3D(conversionFactor * acos(x / divisor),
             conversionFactor * acos(y / divisor),
             conversionFactor * acos(z / divisor));
}

} // Namespace Kernel
} // Namespace Mantid

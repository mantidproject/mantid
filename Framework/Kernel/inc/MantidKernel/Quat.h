// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_QUAT_H_
#define MANTID_KERNEL_QUAT_H_

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
// Forward declarations
class V3D;
template <class T> class Matrix;

/** @class Quat Quat.h Geometry/Quat.h
@brief Class for quaternions
@version 1.0
@author Laurent C Chapon, ISIS RAL
@date 10/10/2007

Templated class for quaternions.
Quaternions are the 3D generalization of complex numbers
Quaternions are used for rotations in 3D spaces and
often implemented for computer graphics applications.
Quaternion can be written q=W+ai+bj+ck where
w is the scalar part, and a, b, c the 3 imaginary parts.
Quaternion multiplication is non-commutative.<br>
i*j=-j*i=k<br>
j*k=-k*j=i<br>
k*i=-i*k=j<br>
Rotation of an angle theta around a normalized axis (u,v,w) can be simply
written W=cos(theta/2), a=u*sin(theta/2), b=v*sin(theta/2), c=w*sin(theta/2)
This class support all arithmetic operations for quaternions
*/
class MANTID_KERNEL_DLL Quat {

public:
  Quat();
  // direct quat definition
  Quat(const double _w, const double _a, const double _b, const double _c);
  // * Construct a Quat between two vectors;
  // * The angle between them is defined differently from usual if vectors are
  // not unit or the same length vectors, so quat would be not consistent
  Quat(const V3D &src, const V3D &des);
  Quat(const V3D &rX, const V3D &rY, const V3D &rZ);
  //! Set quaternion form an angle in degrees and an axis
  Quat(const double _deg, const V3D &_axis);
  // set a quaternion from a rotational matrix;
  Quat(const Matrix<double> &RotMat);
  void operator()(const Quat &);
  void operator()(const double ww, const double aa, const double bb,
                  const double cc);
  void operator()(const double angle, const V3D &);
  void operator()(const V3D &rX, const V3D &rY, const V3D &rZ);

  void set(const double ww, const double aa, const double bb, const double cc);
  void setAngleAxis(const double _deg, const V3D &_axis);
  void getAngleAxis(double &_deg, double &_ax0, double &_ax1,
                    double &ax2) const;
  std::vector<double> getEulerAngles(const std::string &convention) const;
  /// Set the rotation (both don't change rotation axis)
  void setRotation(const double deg);
  //! Norm of a quaternion
  double len() const;
  //! Norm squared
  double len2() const;
  //! Re-initialize to identity
  void init();
  //! Normalize
  void normalize();
  //! Take the complex conjugate
  void conjugate();
  //! Inverse a quaternion (in the sense of rotation inversion)
  void inverse();
  //! Is the quaternion representing a null rotation
  bool isNull(const double tolerance = 0.001) const;
  //! Convert quaternion rotation to an OpenGL matrix [4x4] matrix
  //! stored as an linear array of 16 double
  //! The function glRotated must be called
  void GLMatrix(double *mat) const;
  //! returns the rotation matrix defined by this quaternion as an 9-point
  // vector representing M33 matrix
  //! (m33 is not used at the moment), if check_normalisation selected, verify
  // if the mod(quat) is indeed == 1 and throws otherwise.
  std::vector<double> getRotation(bool check_normalisation = false,
                                  bool throw_on_errors = false) const;
  //! Convert GL Matrix into Quat
  void setQuat(double mat[16]);
  //! Convert usual 3D rotation matrix into quat; Will throw if matirix is not
  // rotational;
  void setQuat(const Matrix<double> &rMat);
  //! Rotate a vector
  void rotate(V3D &) const;

  //! Taking two points defining a cuboid bounding box (xmin,ymin,zmin) and
  //(xmax,ymax,zmax)
  // which means implicitly that the cube edges are parallel to the axes,
  // find the smallest bounding box with the edges also parallel to the axes
  // after rotation of the object.
  void rotateBB(double &xmin, double &ymin, double &zmin, double &xmax,
                double &ymax, double &zmax) const;
  //! Overload operators
  Quat operator+(const Quat &) const;
  Quat &operator+=(const Quat &);
  Quat operator-(const Quat &) const;
  Quat &operator-=(const Quat &);
  Quat operator*(const Quat &)const;
  Quat &operator*=(const Quat &);
  bool operator==(const Quat &) const;
  bool operator!=(const Quat &) const;
  double operator[](int) const;
  double &operator[](int);

  /** @name Element access. */
  //@{
  /// Access the real part
  inline double real() const { return w; }
  /// Access the coefficient of i
  inline double imagI() const { return a; }
  /// Access the coefficient of j
  inline double imagJ() const { return b; }
  /// Access the coefficient of k
  inline double imagK() const { return c; }
  //@}

  void printSelf(std::ostream &) const;
  void readPrinted(std::istream &);
  std::string toString() const;
  void fromString(const std::string &str);

private:
  /// Internal value
  double w;
  /// Internal value
  double a;
  /// Internal value
  double b;
  /// Internal value
  double c;
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const Quat &);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, Quat &q);

} // namespace Kernel

} // namespace Mantid

#endif /*MANTID_KERNEL_QUAT_H_*/

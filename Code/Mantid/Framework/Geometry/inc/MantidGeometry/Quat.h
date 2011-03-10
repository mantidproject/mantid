#ifndef MANTID_QUAT_H_
#define MANTID_QUAT_H_

#include <iostream>
#include <vector>
#include "MantidKernel/System.h"

namespace Mantid
{
  namespace Geometry
  {
    //Forward declarations
    class V3D;
    class M33;

    /** @class Quat Quat.h Geometry/Quat.h
    @brief Class for quaternions
    @version 1.0
    @author Laurent C Chapon, ISIS RAL
    @date 10/10/2007

    Templated class for quaternions.
    Quaternions are the 3D generalization of complex numbers
    Quaternions are used for roations in 3D spaces and
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

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your ption) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    */
    class DLLExport Quat
    {

    public:
      Quat();
      Quat(const double, const double, const double, const double);
      Quat(const V3D& vec1,const V3D& vec2);
      Quat(const V3D& rX, const V3D& rY, const V3D& rZ);
      Quat(const Quat&);
      Quat& operator=(const Quat&);
      //! Set quaternion form an angle in degrees and an axis
      Quat(const double _deg, const V3D& _axis);
      ~Quat();
      void operator()(const Quat&);
      void operator()(const double ww, const double aa, const double bb, const double cc);
      void operator()(const double angle, const V3D&);
      void operator()(const V3D& rX, const V3D& rY, const V3D& rZ);

      // Set quaternion from a 3x3 matrix
      //void operator()(const M33&);
      void set(const double ww, const double aa, const double bb, const double cc);
      void setAngleAxis(const double _deg, const V3D& _axis);
      void getAngleAxis(double& _deg,double& _axis1,double& _axis2,double& axis3) const;
      //void setRotMatrix(const M33&);
      //! Norm of a quaternion
      /// Set the rotation (both don't change rotation axis)
      void setRotation(const double deg);
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
      bool isNull(const double tolerance=0.001) const;
      //! Convert quaternion rotation to an OpenGL matrix [4x4] matrix
      //! stored as an linear array of 16 double
      //! The function glRotated must be called
      void GLMatrix(double* glmat) const;
	  //! returns the rotation matrix defined by this quaternion as an 9-point vector representing M33 matrix 
	  //! (m33 is not used at the moment), if check_normalisation selected, verify if the mod(quat) is indeed == 1 and throws otherwise. 
	  std::vector<double> getRotation(bool check_normalisation=false)const; 
      //! Convert GL Matrix into Quat
      void setQuat(double[16]);
      //! Rotate a vector
      void rotate(V3D&) const;

      //! Taking two points defining a cuboid bounding box (xmin,ymin,zmin) and (xmax,ymax,zmax)
      // which means implicitly that the cube edges are parallel to the axes,
      // find the smallest bounding box with the edges also parallel to the axes after rotation of the object.
      void rotateBB(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax, double& zmax) const;
      //! Overload operators
      Quat  operator+(const Quat&) const;
      Quat& operator+=(const Quat&);
      Quat  operator-(const Quat&) const;
      Quat& operator-=(const Quat&);
      Quat  operator*(const Quat&) const;
      Quat& operator*=(const Quat&);
      bool   operator==(const Quat&) const;
      bool   operator!=(const Quat&) const;
      const double& operator[](int) const;
      double& operator[](int);


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

      void printSelf(std::ostream&) const;
      void readPrinted(std::istream&);
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

    DLLExport std::ostream& operator<<(std::ostream&, const Quat&);
    DLLExport std::istream& operator>>(std::istream&,Quat& q);


  } // Namespace Mantid

} // Namespace Geometry

#endif /*MANTID_QUAT_H_*/

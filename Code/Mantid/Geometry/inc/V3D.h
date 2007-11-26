#ifndef MANTID_V3D_H_
#define MANTID_V3D_H_

#include "MantidKernel/System.h"

namespace Mantid
{
namespace Geometry
{
  class DLLExport V3D
  /** @class V3D 
      @brief Class for 3D vectors. 
      @version A
      @author Laurent C Chapon, ISIS RAL
      @date 09/10/2007
      
      @ignore 
      Copyright 2007 RAL

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
	    
      File change history is stored at: 
      <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  */
    {
      public:

      V3D();
      V3D(const V3D&);
      V3D& operator=(const V3D&);
      V3D(const double,const double,const double);
      V3D(const double*);
      ~V3D();

      // Arithemetic operators overloaded
      V3D operator+(const V3D&) const;
      V3D& operator+=(const V3D&);

      V3D operator-(const V3D&) const;
      V3D& operator-=(const V3D&);
      // Inner product
      V3D operator*(const V3D&) const;
      V3D& operator*=(const V3D&);
      // Inner division
      V3D operator/(const V3D&) const;
      V3D& operator/=(const V3D&);
      // Scale 
      V3D operator*(const double) const;
      V3D& operator*=(const double);		
      V3D operator/(const double) const;
      V3D& operator/=(const double);		
      // Simple Comparison
      bool operator==(const V3D&) const;
      bool operator<(const V3D&) const;
      // Access
      // Setting x, y and z values
      void operator()(const double, const double, const double);
      void spherical(double, double, double);
      // Get x
      const double& X() const { return x; }
      const double& Y() const { return y; } 
      const double& Z() const { return z; }
      
      const double& operator[](const int) const;
      double& operator[](const int);

      // Make a normalized vector (return norm value)
      double normalize();
      double norm() const;    
      double norm2() const; 
      // Scalar product
      double scalar_prod(const V3D&) const;
      // Cross product 
      V3D cross_prod(const V3D&) const;
      // Distance between two points defined as vectors
      double distance(const V3D&) const;
      // Send to a stream
      void printSelf(std::ostream&) const;

    private:

      double x;       ///< X value [unitless]
      double y;       ///< Y value [unitless]
      double z;       ///< Z value [unitless]
   };
	
  // Overload operator <<
  std::ostream& operator<<(std::ostream&, const V3D&);

} // Namespace Geometry
} // Namespace Mantid

#endif /*MANTID_V3D_H_*/

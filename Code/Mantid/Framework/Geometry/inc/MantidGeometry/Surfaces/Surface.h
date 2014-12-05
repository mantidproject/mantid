#ifndef Geometry_Surface_h
#define Geometry_Surface_h

#include "MantidGeometry/DllConfig.h"
#include "BaseVisit.h"
#include <string>

namespace Mantid
{
namespace Kernel
{
  class V3D;
  template<class T>
  class Matrix;
}
namespace Geometry
{


/**
  \class  Surface
  \brief Holds a basic quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a basic surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  
  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  
  File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class MANTID_GEOMETRY_DLL Surface 
{
 private:
  
  int Name;        ///< Surface number (MCNPX identifier)
  
 public:

  static const int Nprecision=10;        ///< Precision of the output

  Surface();
  Surface(const Surface&);
  virtual Surface* clone() const =0;   ///< Abstract clone function
  Surface& operator=(const Surface&);
  virtual ~Surface();

  /// Effective typeid
  virtual std::string className() const { return "Surface"; }
  
  /// Accept visitor for line calculation
  virtual void acceptVisitor(BaseVisit& A) const
  {  A.Accept(*this); }

  void setName(int const N) { Name=N; }            ///< Set Name
  int getName() const { return Name; }             ///< Get Name

  /// Sets the surface based on a string input in MCNPX format
  virtual int setSurface(const std::string& R) =0; 
  virtual int side(const Kernel::V3D&) const;

  /// is point valid on surface 
  virtual int onSurface(const Kernel::V3D& R) const =0;

  /// returns the minimum distance to the surface
  virtual double distance(const Kernel::V3D&) const =0; 
  /// returns the normal to the closest point on the surface
  virtual Kernel::V3D surfaceNormal(const Kernel::V3D&) const =0;

  ///translates the surface
  virtual void displace(const Kernel::V3D&)  =0;
  ///rotates the surface
  virtual void rotate(const Kernel::Matrix<double>&) =0;

  void writeHeader(std::ostream&) const;
  virtual void write(std::ostream&) const;
  virtual void print() const; 
  ///bounding box for the surface
  virtual void getBoundingBox(double& xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin)=0; 

  

};

}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif

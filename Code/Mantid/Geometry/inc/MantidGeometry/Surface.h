#ifndef Geometry_Surface_h
#define Geometry_Surface_h

#include "MantidKernel/System.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "MantidGeometry/BaseVisit.h"

// This is the CORRECT way to do the above then ONLY 
// Surface.cpp requires the long template build to IndexIterator
// and only surface needs the above includes.
namespace Mantid
{
namespace XML
{
  class XMLobject;
  class XMLgroup;
  class XMLcollect;
  template<typename A,typename B> class IndexIterator;
  template<typename XMLobject,typename XMLgroup> class IndexIterator;
}

}

namespace Mantid
{

namespace Geometry
{

  class BaseVisit;

/*!
  \class  Surface
  \brief Holds a basic quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a basic surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  
  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
  
  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
class DLLExport Surface 
{
 private:
  
  /// Static reference to the logger class
  static Kernel::Logger& PLog;

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

  //  virtual double lineIntersect(const Geometry::Vec3D&,
  //			       const Geometry::Vec3D&) const;

  void setName(int const N) { Name=N; }            ///< Set Name
  int getName() const { return Name; }             ///< Get Name

  void matrixForm(Geometry::Matrix<double>&,
		  Geometry::Vec3D&,double&) const;          

  virtual int setSurface(const std::string&) =0; 
  virtual int side(const Geometry::Vec3D&) const;

  // is point valid on surface 
  virtual int onSurface(const Geometry::Vec3D&) const =0;

  virtual double distance(const Geometry::Vec3D&) const =0; 
  virtual Geometry::Vec3D surfaceNormal(const Geometry::Vec3D&) const =0;

  virtual void displace(const Geometry::Vec3D&)  =0;
  virtual void rotate(const Geometry::Matrix<double>&) =0;

  void writeHeader(std::ostream&) const;
  virtual void write(std::ostream&) const;
  virtual void print() const; 


  virtual void procXML(XML::XMLcollect&) const;
  virtual int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,
			int const=0);
  virtual void writeXML(const std::string&) const;

};

}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif

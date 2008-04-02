#ifndef Geometry_Surface_h
#define Geometry_Surface_h

#include "MantidKernel/System.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "BaseVisit.h"

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
  
  double eqnValue(const Geometry::Vec3D&) const;

 protected:

  std::vector<double> BaseEqn;       ///< Base equation (as a 10 point vector)
  
 public:

  static const int Nprecision=10;        ///< Precision of the output

  Surface();
  Surface(const Surface&);
  virtual Surface* clone() const =0;   ///< Abstract clone function
  Surface& operator=(const Surface&);
  virtual ~Surface();

  /// Effective typeid
  virtual std::string className() const { return "Surface"; }

  /// Visit acceptor
  virtual void acceptVisitor(BaseVisit& A) const
    {  A.Accept(*this); }

  //  virtual double lineIntersect(const Geometry::Vec3D&,
  //			       const Geometry::Vec3D&) const;

  void setName(const int N) { Name=N; }            ///< Set Name
  int getName() const { return Name; }             ///< Get Name
  /// access BaseEquation vector
  const std::vector<double>& copyBaseEqn() const { return BaseEqn; } 
  void matrixForm(Geometry::Matrix<double>&,Geometry::Vec3D&,double&) const;          
  virtual int side(const Geometry::Vec3D&) const; 

  virtual void setBaseEqn() =0;      ///< Abstract set baseEqn 

  virtual int onSurface(const Geometry::Vec3D&) const;          ///< is point valid on surface 
  virtual double distance(const Geometry::Vec3D&) const;        ///< distance between point and surface (approx)
  virtual double distanceTrue(const Geometry::Vec3D&) const;    ///< distance between point and surface 
  virtual Geometry::Vec3D surfaceNormal(const Geometry::Vec3D&) const;    ///< Normal at surface

  virtual void displace(const Geometry::Vec3D&);
  virtual void rotate(const Geometry::Matrix<double>&);

  /// Set a surface
  virtual int setSurface(const std::string&) =0;

  void writeHeader(std::ostream&) const;
  virtual void write(std::ostream&) const;
  virtual void print() const; 

  void printGeneral() const;

  virtual void procXML(XML::XMLcollect&) const;
  virtual int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
			const int singleFlag=0);
  virtual void writeXML(const std::string&) const;

};

}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif

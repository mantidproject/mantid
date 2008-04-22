#ifndef Cylinder_h
#define Cylinder_h

#include "MantidKernel/System.h"
#include "MantidGeometry/Quadratic.h"

namespace Mantid
{

namespace Geometry
{

/*!
  \class  Cylinder
  \brief Holds a cylinder as a vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a cylinder as a centre point (on main axis)
  a vector from that point (unit) and a radius.

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

class DLLExport Cylinder : public Quadratic
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
  Geometry::Vec3D Centre;        ///< Geometry::Vec3D for centre
  Geometry::Vec3D Normal;        ///< Direction of centre line
  int Nvec;            ///< Normal vector is x,y or z :: (1-3) (0 if general)
  double Radius;       ///< Radius of cylinder
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);
  void setNvec();      ///< check to obtain orientation

 public:

  /// Public identifer
  virtual std::string className() const { return "Cylinder"; }  

  Cylinder();
  Cylinder(const Cylinder&);
  Cylinder* clone() const;
  Cylinder& operator=(const Cylinder&);
  ~Cylinder();

  // Visit acceptor
  virtual void acceptVisitor(BaseVisit& A) const
    {  A.Accept(*this); }
  
  virtual double lineIntersect(const Geometry::Vec3D&,
			       const Geometry::Vec3D&) const;

  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;
  double distance(const Geometry::Vec3D&) const;

  int setSurface(const std::string&);
  void setCentre(const Geometry::Vec3D&);              
  void setNorm(const Geometry::Vec3D&);       
  Geometry::Vec3D getCentre() const { return Centre; }   ///< Return centre point       
  Geometry::Vec3D getNormal() const { return Normal; }   ///< Return Central line
  double getRadius() const { return Radius; }  ///< Get Radius      
  void setBaseEqn();


  void write(std::ostream&) const;
  void print() const;

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,int const=0);
  //  void writeXML(const std::string&);

};

}   // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif

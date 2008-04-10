#ifndef Cone_h
#define Cone_h

#include "MantidKernel/System.h"

namespace Mantid
{

namespace Geometry 
{

/*!
  \class Cone
  \brief Holds a cone in vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a cone as a centre point (on main axis)
  a vector from that point (unit) and a radius.
  and an angle.

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


class DLLExport Cone : public Quadratic
{
 private:

  static Kernel::Logger& PLog;    ///< The official logger
  
  Geometry::Vec3D Centre;        ///< Geometry::Vec3D for centre
  Geometry::Vec3D Normal;        ///< Normal
  double alpha;                  ///< Angle (degrees)
  double cangle;                 ///< Cos(angle)
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

 public:

  Cone();
  Cone(const Cone&);
  Cone* clone() const;
  Cone& operator=(const Cone&);
  int operator==(const Cone&) const;
  ~Cone();
  
  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;

  /// Return centre point
  Geometry::Vec3D getCentre() const { return Centre; }              
  /// Central normal
  Geometry::Vec3D getNormal() const { return Normal; }       
  /// Edge Angle
  double getCosAngle() const { return cangle; } 
  double distance(const Geometry::Vec3D&) const;   

  int setSurface(const std::string&);
  void setCentre(const Geometry::Vec3D&);              
  void setNorm(const Geometry::Vec3D&);       
  void setAngle(double const);  
  void setTanAngle(double const);
  void setBaseEqn();

  void write(std::ostream&) const;

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,int const=0);

};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif

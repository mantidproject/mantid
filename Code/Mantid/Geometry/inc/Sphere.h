#ifndef Sphere_h
#define Sphere_h

namespace Mantid
{

namespace Geometry
{

/*!
  \class Sphere
  \brief Holds a Sphere as vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a sphere as a centre point and a radius.

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

class Sphere : public Surface
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger  

  Geometry::Vec3D Centre;        ///< Point for centre
  double Radius;                 ///< Radius of sphere
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

 public:
  
  Sphere();
  Sphere(const Sphere&);
  Sphere* clone() const;
  Sphere& operator=(const Sphere&);
  ~Sphere();
  
  int setSurface(const std::string&);
  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;
  double distance(const Geometry::Vec3D&) const;

  void setCentre(const Geometry::Vec3D&);              
  Geometry::Vec3D getCentre() const { return Centre; } ///< Get Centre
  double getRadius() const { return Radius; }          ///< Get Radius
  void setBaseEqn();

  void write(std::ostream&) const; 
  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,const int singleFlag=0);

};

}   // NAMESPACE Geometry

}  // NAMESPACE Mantid
 
#endif

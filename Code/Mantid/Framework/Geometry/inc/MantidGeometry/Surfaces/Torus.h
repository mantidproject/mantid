#ifndef Torus_h
#define Torus_h


namespace Mantid
{

namespace Geometry
{


/*!
  \class Torus
  \brief Holds a torus in vector form
  \author S. Ansell
  \date December 2006
  \version 1.0

  Defines a Torus as a centre, normal and
  three parameters:
  - Iradius :: inner radius of the torus
  - Dradius :: displaced radius (the radius away from the plane)
  - Displacement :: elipse displacment from the centre.
  These are c,b,a in that order.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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


class DLLExport Torus : public Surface
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger

  Geometry::V3D Centre;        ///< Geometry::Vec3D for centre
  Geometry::V3D Normal;        ///< Normal
  double Iradius;                ///< Inner radius
  double Dradius;                ///< Inner radius
  double Displacement;           ///< Displacement

  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::V3D&);

 public:

  /// Public identifier
  virtual std::string className() const { return "Torus"; }

  Torus();
  Torus(const Torus&);
  Torus* clone() const;
  Torus& operator=(const Torus&);
  int operator==(const Torus&) const;
  ~Torus();
  
    /// Accept visitor for line calculation
  virtual void acceptVisitor(BaseVisit& A) const
  {  A.Accept(*this); }

  int setSurface(const std::string& Pstr);
  int side(const Geometry::V3D& R) const;
  int onSurface(const Geometry::V3D& R) const;
  double distance(const Geometry::V3D& Pt) const;   

  /// Return centre point
  Geometry::V3D getCentre() const { return Centre; }              
  /// Central normal
  Geometry::V3D getNormal() const { return Normal; }       
  Geometry::V3D surfaceNormal(const Geometry::V3D& Pt) const;

  void setCentre(const Geometry::V3D& A);              
  void setNorm(const Geometry::V3D& A);

  /// Suppose to set the distance from centre of the torus to the centre of tube. 
  ///TODO:
  void setDistanceFromCentreToTube(double dist);

  /// Suppose to set the radius of the tube which makes up the torus
  ///TODO:
  void setTubeRadius(double dist);

  void write(std::ostream& OX) const;
  void getBoundingBox(double &xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin);
};

} // NAMESPACE

}  // NAMESPACE Mantid

#endif

#ifndef Cone_h
#define Cone_h

#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Logger.h"
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


class DLLExport Cone : public Quadratic
{
 private:

  static Kernel::Logger& PLog;    ///< The official logger
  
  Geometry::V3D Centre;        ///< Geometry::V3D for centre
  Geometry::V3D Normal;        ///< Normal
  double alpha;                  ///< Angle (degrees)
  double cangle;                 ///< Cos(angle)
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::V3D&);

 public:

  /// Public identifer
  virtual std::string className() const { return "Cone"; }  

  Cone();
  Cone(const Cone&);
  Cone* clone() const;
  Cone& operator=(const Cone&);
  int operator==(const Cone&) const;
  ~Cone();
  
	///Calculate if the point R is within the cone (return -1) or outside (return 1)
  int side(const Geometry::V3D& R) const;
	/// Calculate if the point R is on the cone(1=on the surface, 0=not)
  int onSurface(const Geometry::V3D& R) const;

   /// Accept visitor for line calculation
  virtual void acceptVisitor(BaseVisit& A) const
  {  A.Accept(*this); }

  /// Return centre point
  Geometry::V3D getCentre() const { return Centre; }              
  /// Central normal
  Geometry::V3D getNormal() const { return Normal; }       
  /// Edge Angle
  double getCosAngle() const { return cangle; } 
  ///This method returns the distance of the point from the cone
  double distance(const Geometry::V3D&) const;   

  ///This method sets the cone surface using the input string in MCNPx format
  int setSurface(const std::string&);
  ///This method sets the centre of the cone
  void setCentre(const Geometry::V3D&);              
  ///This method sets the cone normal
  void setNorm(const Geometry::V3D&);
  ///This method sets the angle of the cone
  void setAngle(double const);  
  ///This method sets the tan angle which will be converted to cos used for MCNPX format
  void setTanAngle(double const);
  ///This method generates the quadratic equation for cone
  void setBaseEqn();
  ///This method will write the cone equation in MCNP geometry format
  void write(std::ostream&) const;

  ///This will get the bounding box for the cone
  void getBoundingBox(double& xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin);	

  /// The number of slices to approximate a cone
  static int g_nslices;
  /// The number of stacks to approximate a cone
  static int g_nstacks;
};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif

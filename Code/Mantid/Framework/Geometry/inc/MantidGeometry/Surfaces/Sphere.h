#ifndef SPHERE_H
#define SPHERE_H

#include "MantidKernel/System.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/V3D.h"
#include <string>

namespace Mantid
{
  //---------------------------------------------
  // Forward declaration
  //---------------------------------------------
  namespace Kernel
  {
    class Logger;
  }

  namespace Geometry
  {
   /**
    \class Sphere
    \brief Holds a Sphere as vector form
    \author S. Ansell
    \date April 2004
    \version 1.0

    Defines a sphere as a centre point and a radius.

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

    class DLLExport Sphere : public Quadratic
    {
    private:

      static Kernel::Logger& PLog;           ///< The official logger  
      V3D Centre;        ///< Point for centre
      double Radius;                 ///< Radius of sphere
      void rotate(const Geometry::Matrix<double>&);
      void displace(const Geometry::V3D&);
      /// Compute the distance from the centre of the sphere to the given point
      double centreToPoint(const V3D & pt) const;

    public:
      Sphere();
      Sphere(const Sphere&);
      Sphere* clone() const;
      Sphere& operator=(const Sphere&);
      ~Sphere();
      /// Effective typename 
      virtual std::string className() const { return "Sphere"; }
      // Visit acceptor
      virtual void acceptVisitor(BaseVisit& A) const
      {  A.Accept(*this); }
      ///Set the sphere defination by input string in MCNP format	
      int setSurface(const std::string&);
      ///Checks the given input point to be inside, outside or on the surface of sphere
      int side(const Geometry::V3D&) const;
      ///Checks whether the give input point is on the surface
      int onSurface(const Geometry::V3D&) const;
      ///Gets the distance from the sphere to the input point
      double distance(const Geometry::V3D&) const;
      ///Setter for centre of sphere
      void setCentre(const Geometry::V3D&);              
      /// Get Centre
      Geometry::V3D getCentre() const { return Centre; } 
      /// Get Radius
      double getRadius() const { return Radius; }      
      /// Set Radius
      void setRadius(const double& r) { Radius=r; setBaseEqn();}   
      ///Generates the quadratic equation.
      void setBaseEqn();
      ///Writes the sphere equatation in MCNP format
      void write(std::ostream&) const; 
      void getBoundingBox(double &xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin);

      /// The number of slices to approximate a sphere
      static int g_nslices;
      /// The number of stacks to approximate a sphere
      static int g_nstacks;
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif

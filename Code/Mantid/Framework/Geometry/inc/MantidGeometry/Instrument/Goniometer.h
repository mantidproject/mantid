#ifndef MANTID_GEOMETRY_GONIOMETER_H_
#define MANTID_GEOMETRY_GONIOMETER_H_

#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include <string>
#include <nexus/NeXusFile.hpp>

namespace Mantid
{
namespace Geometry
{

/**
  Class to represent a particular goniometer setting, which is described by the rotation matrix.
  For a particular sample environment, it stores the rotation motors (names, angles, ...). If copied from 
  one workspace to another, one might need to just change the angle value for one particular axis.

  @class Goniometer
  @author Andrei Savici, SNS, ORNL
  @date 04/13/2011

  Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

  enum RotationSense{
    CW=-1,///Clockwise rotation
    CCW=1};///Counter clockwise rotation

  /// Structure containing a rotation axis: name, direction, angle, sense
  struct GoniometerAxis
  {
    std::string name; /// GoniometerAxis name
    Kernel::V3D rotationaxis; /// GoniometerAxis direction
    double angle; /// Rotation angle
    int sense;  /// Rotation sense (1 for CCW, -1 for CW)
    int angleunit; ///angle units are angDegrees or angRadians (see AngleUnits.h)
    /// Constructor
    GoniometerAxis(std::string initname, Kernel::V3D initrotationaxis,double initangle,int initsense,int initangleunit):name(initname),rotationaxis(initrotationaxis),angle(initangle),sense(initsense),angleunit(initangleunit){}
    GoniometerAxis() : name("") {}

    void saveNexus(::NeXus::File * file, const std::string & group) const;
    void loadNexus(::NeXus::File * file, const std::string & group);
  };


  class MANTID_GEOMETRY_DLL Goniometer
  {
    public:
      // Default constructor
      Goniometer();
      // Copy constructor
      Goniometer(const Goniometer& other);
      // Constructor from a rotation matrix
      Goniometer(Kernel::DblMatrix rot);
      // Default destructor
      virtual ~Goniometer();
      // Return rotation matrix
      const Kernel::DblMatrix& getR() const;
      // Set the rotation matrix
      void setR(Kernel::DblMatrix rot);
      // Return information about axes  
      std::string axesInfo();
      // Add axis to goniometer
      void pushAxis(std::string name, double axisx, double axisy, double axisz, double angle=0., int sense=CCW, int angUnit=angDegrees);
      // Set rotation angle for an axis in the units the angle is set (default -- degrees)
      void setRotationAngle( std::string name, double value);
      // Set rotation angle for an axis in the units the angle is set (default -- degrees)
      void setRotationAngle( size_t axisnumber, double value);
      // Get axis object
      const GoniometerAxis & getAxis(size_t axisnumber) const;
      // Get axis object
      const GoniometerAxis & getAxis(std::string axisname) const;
      // Return the number of axes
      size_t getNumberAxes() const;
      // Make a default universal goniometer
      void makeUniversalGoniometer();
      // Return Euler angles acording to a convention
      std::vector<double> getEulerAngles(std::string convention="YZX");

      void saveNexus(::NeXus::File * file, const std::string & group) const;
      void loadNexus(::NeXus::File * file, const std::string & group);
      /// the method reports if the goniometer was defined with some parameters
      bool isDefined()const;
    private:
      /// Global rotation matrix of the goniometer
      Kernel::DblMatrix R;
      /// Motors vector contains GoniometerAxis objects, the last one is the closest to the sample
      std::vector<GoniometerAxis> motors;
      /// Flag to specify if the goniometer is initialized from a rotation matrix
      bool initFromR;
      /// Private function to recalculate R when setRotationAngle is called
      void recalculateR();
  }; 
} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_GONIOMETER_H_*/

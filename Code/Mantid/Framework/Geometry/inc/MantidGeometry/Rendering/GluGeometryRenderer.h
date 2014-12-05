#ifndef GLU_GEOMETRYRENDERER_H
#define GLU_GEOMETRYRENDERER_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid
{
  namespace Kernel
  {
    class V3D;
  }
  namespace Geometry
  {
     class IObjComponent;
    /**
       \class GluGeometryRenderer
       \brief rendering geometry using opengl utility library glu.
       \author Srikanth Nagella
       \date July 2008
       \version 1.0

       This is an concrete class for rendering geometry using opengl.

       Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_GEOMETRY_DLL GluGeometryRenderer
    {
    public:
      GluGeometryRenderer();       ///< Constructor
      ~GluGeometryRenderer();      ///< Destructor
      ///Renders an object component
      void Render(IObjComponent* ObjComp) const;
      ///Renders a Sphere from the input values
      void RenderSphere(const Kernel::V3D& center,double radius);
      ///Renders a Cuboid from the input values
      void RenderCube(const Kernel::V3D& Point1,const Kernel::V3D& Point2,const Kernel::V3D& Point3,const Kernel::V3D& Point4);
      ///Renders a Cone from the input values
      void RenderCone(const Kernel::V3D& center,const Kernel::V3D &axis,double radius,double height);
      ///Renders a Cylinder from the input values
      void RenderCylinder(const Kernel::V3D& center,const Kernel::V3D& axis,double radius,double height);
      ///Renders a Segmented Cylinder from the input values
      void RenderSegmentedCylinder(const Kernel::V3D& center,const Kernel::V3D& axis,double radius,double height);
      ///Creates a sphere from the input values
      void CreateSphere(const Kernel::V3D& center,double radius);
      ///Creates a cuboid from the input values
      void CreateCube(const Kernel::V3D& Point1,const Kernel::V3D& Point2,const Kernel::V3D& Point3,const Kernel::V3D& Point4);
      ///Creates a Cone from the input values
      void CreateCone(const Kernel::V3D& center,const Kernel::V3D& axis,double radius,double height);
      ///Creates a cylinder from the input values
      void CreateCylinder(const Kernel::V3D& center,const Kernel::V3D& axis,double radius,double height);
      ///Creates a segmented cylinder from the input values
      void CreateSegmentedCylinder(const Kernel::V3D& center,const Kernel::V3D& axis,double radius,double height);
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif

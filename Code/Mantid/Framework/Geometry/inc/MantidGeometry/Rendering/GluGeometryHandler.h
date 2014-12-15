#ifndef GLU_GEOMETRYHANDLER_H
#define GLU_GEOMETRYHANDLER_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

namespace Mantid
{
  namespace Kernel
  {
    class V3D;
  }
  namespace Geometry
  {
    class GeometryHandler;
    class GluGeometryRenderer;
    class IObjComponent;
    class Object;
    /**
       \class GluGeometryHandler
       \brief Place holder for geometry triangulation and rendering with special cases of cube, sphere, cone and cylinder.
       \author Srikanth Nagella
       \date December 2008
       \version 1.0

       This is an implementation class for handling geometry without triangulating and using opengl glu methods.
       This class can render cube, sphere, cone and cylinder.

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
    class MANTID_GEOMETRY_DLL GluGeometryHandler: public GeometryHandler
    {
      /// the type of the geometry eg CUBOID,CYLINDER,CONE,SPHERE
      enum GEOMETRY_TYPE
      {
        CUBOID,   ///< CUBOID
        CYLINDER, ///< CYLINDER
        CONE,     ///< CONE
        SPHERE,    ///< SPHERE
        SEGMENTED_CYLINDER ///< Cylinder with 1 or more segments (along the axis). Sizes of segments are important.
      }; 
    private:
      static Kernel::Logger& PLog;           ///< The official logger
      GluGeometryRenderer* Renderer;         ///< Geometry renderer variable used for rendering Object/ObjComponent

      Kernel::V3D center; ///<Center for sphere,cone and cylinder
      Kernel::V3D Point1; ///<cube coordinates
      Kernel::V3D Point2; ///<cube coordinates
      Kernel::V3D Point3; ///<cube coordinates
      Kernel::V3D Point4; ///<cube coordinates
      double radius; ///<Radius for the sphere, cone and cylinder
      double height; ///<height for cone and cylinder;
      Kernel::V3D axis; ///<  Axis
      GEOMETRY_TYPE type; ///< the type of the geometry eg CUBOID,CYLINDER,CONE,SPHERE
    public:
      GluGeometryHandler(IObjComponent* obj); ///< Constructor
      GluGeometryHandler(boost::shared_ptr<Object> obj); ///< Constructor
      GluGeometryHandler(Object* obj); ///< Constructor
      boost::shared_ptr<GeometryHandler> clone() const;
      ~GluGeometryHandler(); ///< Destructor
      GeometryHandler* createInstance(IObjComponent *comp);
      GeometryHandler* createInstance(boost::shared_ptr<Object> obj);
      GeometryHandler* createInstance(Object*);
      ///sets the geometry handler for a cuboid
      void setCuboid(Kernel::V3D,Kernel::V3D,Kernel::V3D,Kernel::V3D);
      ///sets the geometry handler for a cone
      void setSphere(Kernel::V3D,double);
      ///sets the geometry handler for a cylinder
      void setCylinder(Kernel::V3D,Kernel::V3D,double,double);
      ///sets the geometry handler for a cone
      void setCone(Kernel::V3D,Kernel::V3D,double,double);
      ///sets the geometry handler for a segmented cylinder
      void setSegmentedCylinder(Kernel::V3D,Kernel::V3D,double,double);
      void Triangulate();
      void Render();
      void Initialize();
      void GetObjectGeom(int& mytype, std::vector<Kernel::V3D>& vectors, double& myradius, double & myheight);
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif

#ifndef OC_GEOMETRYHANDLER_H
#define OC_GEOMETRYHANDLER_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid {

namespace Geometry {
class GeometryHandler;
class OCGeometryRenderer;
class OCGeometryGenerator;
class IObjComponent;
class CSGObject;
/**
   \class OCGeometryHandler
   \brief Place holder for OpenCascade library geometry triangulation and
   rendering.
   \author Srikanth Nagella
   \date July 2008
   \version 1.0

   This is an implementation class for handling geometry using
   OpenCascade(www.opencascade.org).
   Unlike the GluGeometryHandler, it can handle more complex shapes.
   It uses OpenCascade to generate a mesh defining the surface of an Object
   (shape).
   This shape is saved along with the instrument definition as a .vtp file (for
   speed-ups).

   Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL OCGeometryHandler : public GeometryHandler {
private:
  static Kernel::Logger &PLog;  ///< The official logger
  OCGeometryRenderer *Renderer; ///< Geometry renderer variable used for
  /// rendering Object/ObjComponent
  OCGeometryGenerator *
      Triangulator; ///< Geometry generator to triangulate Object
public:
  OCGeometryHandler(IObjComponent *comp);              ///< Constructor
  OCGeometryHandler(boost::shared_ptr<CSGObject> obj); ///< Constructor
  OCGeometryHandler(CSGObject *obj);                   ///< Constructor
  boost::shared_ptr<GeometryHandler>
  clone() const override;        ///< Virtual copy constructor
  ~OCGeometryHandler() override; ///< Destructor
  GeometryHandler *createInstance(IObjComponent *comp) override;
  GeometryHandler *createInstance(boost::shared_ptr<CSGObject> obj) override;
  GeometryHandler *createInstance(CSGObject *) override;
  void Triangulate() override;
  void Render() override;
  void Initialize() override;
  /// Returns true if the shape can be triangulated
  bool canTriangulate() override { return true; }
  /// get the number of Triangles
  int NumberOfTriangles() override;
  /// get the number of points or vertices
  int NumberOfPoints() override;
  /// Extract the vertices of the triangles
  double *getTriangleVertices() override;
  /// Extract the Faces of the triangles
  int *getTriangleFaces() override;
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif

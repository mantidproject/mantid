#ifndef OC_GEOMETRYGENERATOR_H
#define OC_GEOMETRYGENERATOR_H

#include "MantidGeometry/DllConfig.h"

class TopoDS_Shape;

namespace Mantid {
namespace Geometry {
class CSGObject;
class Intersection;
class Union;
class SurfPoint;
class CompGrp;
class CompObj;
class BoolValue;
class Rule;
class Surface;
class Cylinder;
class Sphere;
class Cone;
class Plane;
class Torus;

/**
   \class OCGeometryGenerator
   \brief Generates OpenCascade geometry from the ObjComponent
   \author Mr. Srikanth Nagella
   \date 4.08.2008
   \version 1.0

   This class is an OpenCascade geometry generation that takes in input as
   ObjComponent.

   Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL OCGeometryGenerator {
private:
  const CSGObject *Obj;     ///< Input Object
  TopoDS_Shape *ObjSurface; ///< Storage for the output surface
  /// Analyze the object
  void AnalyzeObject();

public:
  OCGeometryGenerator(const CSGObject *obj);
  ~OCGeometryGenerator();
  void Generate();
  TopoDS_Shape *getObjectSurface();
  /// return number of triangles in mesh (0 for special shapes)
  int getNumberOfTriangles();
  /// return number of points used in mesh (o for special shapes)
  int getNumberOfPoints();
  /// get a pointer to the 3x(NumberOfPoints) coordinates (x1,y1,z1,x2..) of
  /// mesh
  double *getTriangleVertices();
  /// get a pointer to the 3x(NumberOFaces) integers describing points forming
  /// faces (p1,p2,p3)(p4,p5,p6).
  int *getTriangleFaces();
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif

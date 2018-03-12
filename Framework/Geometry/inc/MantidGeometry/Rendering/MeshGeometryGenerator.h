#ifndef MESH_GEOMETRYGENERATOR_H
#define MESH_GEOMETRYGENERATOR_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid {

namespace Geometry {

/**
   This class delivers the triangles of a MeshObject for rendering via
   the CacheGeometryRenderer.

   Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MeshObject;
class MANTID_GEOMETRY_DLL MeshGeometryGenerator {
private:
  MeshObject *Obj;    ///< Input Object
  int mNoOfVertices;  ///< number of vertices
  int mNoOfTriangles; ///< number of triangles
  double *mPoints;    ///<double array or points
  int *mFaces;        ///< Integer array of faces
public:
  MeshGeometryGenerator(MeshObject *obj);
  ~MeshGeometryGenerator();
  /// Generate the trangles
  void Generate();
  /// get the number of triangles
  int getNumberOfTriangles();
  /// get the number of points
  int getNumberOfPoints();
  /// get the triangle vertices
  double *getTriangleVertices();
  /// get the triangle faces
  int *getTriangleFaces();
  /// Sets the geometry cache using the triangulation information provided
  void setGeometryCache(int noPts, int noFaces, double *pts, int *faces);
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif MESH_GEOMETRYGENERATOR_H

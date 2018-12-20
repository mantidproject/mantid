#ifndef MANTID_GEOMETRY_SURFACETRIANGULATOR_H_
#define MANTID_GEOMETRY_SURFACETRIANGULATOR_H_

#include "MantidGeometry/DllConfig.h"
#include <memory>
#include <vector>

class TopoDS_Shape;

namespace Mantid {
namespace Geometry {
class CSGObject;
class RenderingMesh;

namespace detail {

/** GeometryTriangulator : Triangulates object surfaces. May or may not use
  opencascade.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL GeometryTriangulator {
private:
  bool m_isTriangulated;
  size_t m_nFaces = 0;
  size_t m_nPoints = 0;
  std::vector<double> m_points;        ///< double array or points
  std::vector<uint32_t> m_faces;       ///< Integer array of faces
  const CSGObject *m_csgObj = nullptr; ///< Input Object
  std::unique_ptr<RenderingMesh> m_meshObj;
  void checkTriangulated();

public:
  GeometryTriangulator(const CSGObject *obj = nullptr);
  GeometryTriangulator(std::unique_ptr<RenderingMesh> obj);
  GeometryTriangulator(const GeometryTriangulator &) = delete;
  GeometryTriangulator &operator=(const GeometryTriangulator &) = delete;
  ~GeometryTriangulator();
  void triangulate();
  void generateMesh();
  void setGeometryCache(size_t nPoints, size_t nFaces,
                        std::vector<double> &&points,
                        std::vector<uint32_t> &&faces);
  /// Return the number of triangle faces
  size_t numTriangleFaces();
  /// Return the number of triangle vertices
  size_t numTriangleVertices();
  /// get a pointer to the 3x(NumberOfPoints) coordinates (x1,y1,z1,x2..) of
  /// mesh
  const std::vector<double> &getTriangleVertices();
  /// get a pointer to the 3x(NumberOFaces) integers describing points forming
  /// faces (p1,p2,p3)(p4,p5,p6).
  const std::vector<uint32_t> &getTriangleFaces();
#ifdef ENABLE_OPENCASCADE
private:
  std::unique_ptr<TopoDS_Shape>
      m_objSurface; ///< Storage for the output surface
                    /// Analyze the object
                    /// OpenCascade analysis of object surface
  void OCAnalyzeObject();
  size_t numPoints() const;
  size_t numFaces() const;
  void setupPoints();
  void setupFaces();

public:
  /// Return OpenCascade surface.
  bool hasOCSurface() const;
  const TopoDS_Shape &getOCSurface();
#endif
};
} // namespace detail
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SURFACETRIANGULATOR_H_ */

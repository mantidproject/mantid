#ifndef MANTID_GEOMETRY_GEOMETRYRENDERER_H_
#define MANTID_GEOMETRY_GEOMETRYRENDERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"

class TopoDS_Shape;

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class RectangularDetector;
class StructuredDetector;
class IObjComponent;

/** GeometryRenderer : Handles rendering of geometry within mantid.

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
class MANTID_GEOMETRY_DLL GeometryRenderer {
public:
  enum class RenderMode { Basic, Volumetric };
  GeometryRenderer() = default;
  ~GeometryRenderer() = default;

  /// General method for rendering geometry
  template <typename... Args>
  void render(RenderMode mode, Args &&... args) const &;

  /// Render Basic geometry without transparency (non-volumetric)
  template <typename... Args> void render(Args &&... args) const &;

  /// Render IObjComponent
  void renderIObjComponent(IObjComponent *objComp,
                           RenderMode mode = RenderMode::Basic) const;
  /// Render Traingulated Surface
  void renderTriangulated(int noPts, int noFaces, double *points, int *faces,
                          RenderMode mode = RenderMode::Basic) const;
  /// Render OpenCascade Shape
  void renderOpenCascade(TopoDS_Shape *objSurf,
                         RenderMode mode = RenderMode::Basic) const;
  /// Renders a sphere
  void renderSphere(const Kernel::V3D &center, double radius,
                    RenderMode mode = RenderMode::Basic) const;
  /// Renders a cuboid
  void renderCuboid(const Kernel::V3D &Point1, const Kernel::V3D &Point2,
                    const Kernel::V3D &Point3, const Kernel::V3D &Point4,
                    RenderMode mode = RenderMode::Basic) const;
  /// Renders a Hexahedron from the input values
  void renderHexahedron(const std::vector<Kernel::V3D> &points,
                        RenderMode mode = RenderMode::Basic) const;
  /// Renders a Cone from the input values
  void renderCone(const Kernel::V3D &center, const Kernel::V3D &axis,
                  double radius, double height,
                  RenderMode mode = RenderMode::Basic) const;
  /// Renders a Cylinder/Segmented cylinder from the input values
  void renderCylinder(const Kernel::V3D &center, const Kernel::V3D &axis,
                      double radius, double height, bool segmented = false,
                      RenderMode mode = RenderMode::Basic) const;
  /// Renders a Bitmap (used for rendering RectangularDetector)
  void renderBitmap(const RectangularDetector *rectDet,
                    RenderMode mode = RenderMode::Basic) const;
  /// Renders structured geometry (used for rendering StructuredDetector)
  void renderStructured(const StructuredDetector *structDet,
                        RenderMode mode = RenderMode::Basic) const;

private:
  mutable RenderMode m_renderMode;
  // general geometry
  /// Render IObjComponent
  void doRender(IObjComponent *ObjComp) const;
  /// Render Traingulated Surface
  void doRender(int noPts, int noFaces, double *points, int *faces) const;
  /// Render OpenCascade Shape
  void doRender(TopoDS_Shape *ObjSurf) const;

  // shapes
  /// Renders a sphere
  void doRender(const Kernel::V3D &center, double radius) const;
  /// Renders a cuboid
  void doRender(const Kernel::V3D &Point1, const Kernel::V3D &Point2,
                const Kernel::V3D &Point3, const Kernel::V3D &Point4) const;
  /// Renders a Hexahedron from the input values
  void doRender(const std::vector<Kernel::V3D> &points) const;
  /// Renders a Cone from the input values
  void doRender(const Kernel::V3D &center, const Kernel::V3D &axis,
                double radius, double height) const;
  /// Renders a Cylinder/Segmented cylinder from the input values
  void doRender(const Kernel::V3D &center, const Kernel::V3D &axis,
                double radius, double height, bool segmented) const;
  /// Renders a Bitmap (used for rendering RectangularDetector)
  void doRender(const RectangularDetector *rectDet) const;
  /// Renders structured geometry (used for rendering StructuredDetector)
  void doRender(const StructuredDetector *structDet) const;
};

template <typename... Args>
void GeometryRenderer::render(RenderMode mode, Args &&... args) const & {
  // Wait for no OopenGL error
  while (glGetError() != GL_NO_ERROR)
    ;
  m_renderMode = mode;
  doRender(std::forward<Args>(args)...);
}

template <typename... Args>
void GeometryRenderer::render(Args &&... args) const & {
  render(RenderMode::Basic, std::forward<Args>(args)...);
}

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_GEOMETRYRENDERER_H_ */
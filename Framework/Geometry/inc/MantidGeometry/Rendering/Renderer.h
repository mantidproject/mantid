#ifndef MANTID_GEOMETRY_RENDERER_H_
#define MANTID_GEOMETRY_RENDERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include <vector>

class TopoDS_Shape;

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class RectangularDetector;
class StructuredDetector;
class IObjComponent;

  namespace detail
  {
    class GeometryTriangulator;
  }

/** Renderer : Handles rendering details of geometry within mantid.

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
namespace detail {
class ShapeInfo;

class MANTID_GEOMETRY_DLL Renderer {
public:
  enum class RenderMode { Basic, Volumetric };
  Renderer() = default;
  ~Renderer() = default;

  /// General method for rendering geometry
  template <typename... Args>
  void render(RenderMode mode, Args &&... args) const &;

  /// Render Basic geometry without transparency (non-volumetric)
  template <typename... Args> void render(Args &&... args) const &;

  /// Render IObjComponent
  void renderIObjComponent(const IObjComponent &objComp,
                           RenderMode mode = RenderMode::Basic) const;
  /// Render Traingulated Surface
  void renderTriangulated(detail::GeometryTriangulator &triangulator,
                          RenderMode mode = RenderMode::Basic) const;
  /// Renders a sphere, cuboid, hexahedron, cone or cylinder
  void renderShape(const ShapeInfo &shapeInfo) const;
  /// Renders a Bitmap (used for rendering RectangularDetector)
  void renderBitmap(const RectangularDetector &rectDet,
    RenderMode mode = RenderMode::Basic) const;
  /// Renders structured geometry (used for rendering StructuredDetector)
  void renderStructured(const StructuredDetector &structDet,
    RenderMode mode = RenderMode::Basic) const;
private:
  mutable RenderMode m_renderMode;
  /// Renders a sphere
  void doRenderSphere(const ShapeInfo &shapeInfo) const;
  /// Renders a cuboid
  void doRenderCuboid(const ShapeInfo &shapeInfo) const;
  /// Renders a Hexahedron from the input values
  void doRenderHexahedron(const ShapeInfo &shapeInfo) const;
  /// Renders a Cone from the input values
  void doRenderCone(const ShapeInfo &shapeInfo) const;
  /// Renders a Cylinder/Segmented cylinder from the input values
  void doRenderCylinder(const ShapeInfo &shapeInfo) const;
  // general geometry
  /// Render IObjComponent
  void doRender(const IObjComponent &ObjComp) const;
  /// Render Traingulated Surface
  void doRender(GeometryTriangulator &triangulator) const;
#ifdef ENABLE_OPENCASCADE
  /// Render OpenCascade Shape
  void doRender(const TopoDS_Shape &ObjSurf) const;
#endif
  /// Renders a Bitmap (used for rendering RectangularDetector)
  void doRender(const RectangularDetector &rectDet) const;
  /// Renders structured geometry (used for rendering StructuredDetector)
  void doRender(const StructuredDetector &structDet) const;
};

template <typename... Args>
void Renderer::render(RenderMode mode, Args &&... args) const & {
  // Wait for no OopenGL error
  while (glGetError() != GL_NO_ERROR)
    ;
  m_renderMode = mode;
  doRender(std::forward<Args>(args)...);
}

template <typename... Args> void Renderer::render(Args &&... args) const & {
  render(RenderMode::Basic, std::forward<Args>(args)...);
}
} // namespace detail
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_RENDERER_H_ */
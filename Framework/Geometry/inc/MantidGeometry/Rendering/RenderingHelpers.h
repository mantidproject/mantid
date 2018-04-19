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

namespace detail {
class GeometryTriangulator;
class ShapeInfo;
} // namespace detail

namespace RenderingHelpers {
/// Render IObjComponent
MANTID_GEOMETRY_DLL void renderIObjComponent(const IObjComponent &objComp);
/// Render Traingulated Surface
MANTID_GEOMETRY_DLL void
renderTriangulated(detail::GeometryTriangulator &triangulator);
/// Renders a sphere, cuboid, hexahedron, cone or cylinder
MANTID_GEOMETRY_DLL void renderShape(const detail::ShapeInfo &shapeInfo);
/// Renders a Bitmap (used for rendering RectangularDetector)
MANTID_GEOMETRY_DLL void renderBitmap(const RectangularDetector &rectDet);
/// Renders structured geometry (used for rendering StructuredDetector)
MANTID_GEOMETRY_DLL void renderStructured(const StructuredDetector &structDet);
} // namespace RenderingHelpers
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_RENDERER_H_ */
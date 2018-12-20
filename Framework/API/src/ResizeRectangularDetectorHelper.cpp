#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"

namespace Mantid {

namespace API {

void applyRectangularDetectorScaleToComponentInfo(
    Geometry::ComponentInfo &componentInfo, Geometry::ComponentID componentId,
    const double scaleX, const double scaleY) {

  const size_t componentIndex = componentInfo.indexOf(componentId);
  // Precompute transformation: Undo translation, undo rotation, scale, rotate
  // back, translate back:
  Eigen::Affine3d transformation(
      Eigen::Scaling(Eigen::Vector3d(scaleX, scaleY, 1)));
  const auto rotation = toQuaterniond(componentInfo.rotation(componentIndex));
  transformation.rotate(rotation.conjugate());
  transformation.prerotate(rotation);
  const auto origin =
      Kernel::toVector3d(componentInfo.position(componentIndex));
  transformation.translate(-origin);
  transformation.pretranslate(origin);

  for (auto index : componentInfo.detectorsInSubtree(componentIndex)) {
    auto newPos = Kernel::toV3D(
        transformation * Kernel::toVector3d(componentInfo.position(index)));
    componentInfo.setPosition(index, newPos);
  }
}

} // namespace API
} // namespace Mantid

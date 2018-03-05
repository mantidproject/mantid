#ifndef SHAPE_GEOMETRY_ABSTRACTION_H_
#define SHAPE_GEOMETRY_ABSTRACTION_H_
//------------------
// Includes
//------------------

#include "MantidNexusGeometry/DllConfig.h"

#include "Eigen/Core"

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class Surface;
class IObject;
}
namespace NexusGeometry {

// TODO delete
typedef boost::shared_ptr<const Geometry::IObject> objectHolder;
// TODO delete
typedef boost::shared_ptr<Geometry::Surface> surfaceHolder;

namespace NexusShapeFactory {
DLLExport objectHolder
createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef);
DLLExport objectHolder createMesh(std::vector<uint16_t> &&triangularFaces,
                                  std::vector<Mantid::Kernel::V3D> &&vertices);
}
}
}
#endif // SHAPE_GEOMETRY_ABSTRACTION_H_

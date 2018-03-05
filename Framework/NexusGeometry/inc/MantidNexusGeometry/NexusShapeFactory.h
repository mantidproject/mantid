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
class IObject;
}
namespace NexusGeometry {

namespace NexusShapeFactory {
DLLExport boost::shared_ptr<const Geometry::IObject>
createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef);
DLLExport boost::shared_ptr<const Geometry::IObject>
createMesh(std::vector<uint16_t> &&triangularFaces,
           std::vector<Mantid::Kernel::V3D> &&vertices);
}
}
}
#endif // SHAPE_GEOMETRY_ABSTRACTION_H_

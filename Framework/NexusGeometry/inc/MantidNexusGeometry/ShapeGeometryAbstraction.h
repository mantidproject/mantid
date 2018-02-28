#ifndef SHAPE_GEOMETRY_ABSTRACTION_H_
#define SHAPE_GEOMETRY_ABSTRACTION_H_
//------------------
// Includes
//------------------

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidNexusGeometry/DllConfig.h"
#include "MantidNexusGeometry/ShapeAbstractCreator.h"

#include "Eigen/Core"

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

typedef boost::shared_ptr<Geometry::IObject> objectHolder;
typedef boost::shared_ptr<Geometry::Surface> surfaceHolder;

class DLLExport ShapeGeometryAbstraction
    : public ShapeAbstractCreator<ShapeGeometryAbstraction, objectHolder> {
public:
  objectHolder createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef);
  objectHolder createMesh(std::vector<uint16_t> &&triangularFaces,
                          std::vector<Mantid::Kernel::V3D> &&vertices);

private:
  objectHolder createShape(const std::map<int, surfaceHolder> &surfaces,
                           const std::string &algebra,
                           std::vector<double> &boundingBox);
};
}
}
#endif // SHAPE_GEOMETRY_ABSTRACTION_H_

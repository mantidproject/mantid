#ifndef SHAPE_GEOMETRY_ABSTRACTION_H_
#define SHAPE_GEOMETRY_ABSTRACTION_H_
//------------------
// Includes
//------------------

#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidNexusGeometry/DllConfig.h"
#include "MantidNexusGeometry/ShapeAbstractCreator.h"

#include "Eigen/Core"

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

typedef boost::shared_ptr<Geometry::Object> objectHolder;
typedef boost::shared_ptr<Geometry::Surface> surfaceHolder;

class DLLExport ShapeGeometryAbstraction
    : public ShapeAbstractCreator<ShapeGeometryAbstraction, objectHolder> {
public:
  objectHolder createCylinder(Eigen::Matrix<double, 3, 3> &pointsDef);

private:
  objectHolder createShape(std::map<int, surfaceHolder> &surfaces,
                           std::string &algebra,
                           std::vector<double> &boundingBox);
};
}
}
#endif // SHAPE_GEOMETRY_ABSTRACTION_H_

#ifndef SHAPE_ABSTRACT_CREATOR_H_
#define SHAPE_ABSTRACT_CREATOR_H_

//-----------------------
// Includes
//-----------------------

#include "Eigen/Core"

namespace Mantid {
namespace NexusGeometry {

// Instrument Abstract Builder crpt class
template <typename shapeAbstractionClass, typename shapeAbstractionObject>
class ShapeAbstractCreator {
public:
  /// Make a cylinder
  shapeAbstractionObject
  createCylinder(Eigen::Matrix<double, 3, 3> &pointsDef) {
    this->shapeImp().createCylinder(pointsDef);
  }

private:
  /// Factor out the static_cast
  shapeAbstractionClass &shapeImp() {
    return static_cast<shapeAbstractionClass &>(*this);
  }
};
}
}

#endif // SHAPE_ABSTRACT_CREATOR_H_

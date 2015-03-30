#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCE_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCE_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/VectorParameter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace DataObjects {
/// Unique CoordCenterVectorParam type declaration for ndimensional coordinate
/// centers
DECLARE_VECTOR_PARAMETER(CoordCenterVectorParam, coord_t)

/// Unique DimensionsUsedVectorParam type declaration for boolean masks over
/// dimensions
DECLARE_VECTOR_PARAMETER(DimensionsUsedVectorParam, bool)

/** A non-linear coordinate transform that takes
 * a point from nd dimensions and converts it to a
 * single dimension: the SQUARE of the distance between the MDLeanEvent
 * and a given point in up to nd dimensions.
 *
 * The number of output dimensions is 1 (the square of the distance to the
 *point).
 *
 * The square is used instead of the plain distance since square root is a slow
 *calculation.
 *
 * @author Janik Zikovsky
 * @date 2011-04-25 14:48:33.517020
 */
class DLLExport CoordTransformDistance : public Mantid::API::CoordTransform {
public:
  CoordTransformDistance(const size_t inD, const coord_t *center,
                         const bool *dimensionsUsed, const size_t outD = 1);
  virtual CoordTransform *clone() const;
  virtual ~CoordTransformDistance();
  virtual std::string toXMLString() const;
  virtual std::string id() const;

  void apply(const coord_t *inputVector, coord_t *outVector) const;

  /// Return the center coordinate array
  const coord_t *getCenter() { return m_center; }

  /// Return the dimensions used bool array
  const bool *getDimensionsUsed() { return m_dimensionsUsed; }

protected:
  /// Coordinates at the center
  coord_t *m_center;

  /// Parmeter where True is set for those dimensions that are considered when
  /// calculating distance
  bool *m_dimensionsUsed;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCE_H_ */

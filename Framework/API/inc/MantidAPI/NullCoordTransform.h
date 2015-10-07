#ifndef MANTIDAPI_NULLCOORDTRANSFORM_H
#define MANTIDAPI_NULLCOORDTRANSFORM_H

#include "MantidAPI/CoordTransform.h"

namespace Mantid {
namespace API {

/** NullCoordTransform : A transform that sets the outVector to have the same
 * values as the inputVector.
 * Therefore has no-effect, for where transforms are not required.
 * @author Owen Arnold
 * @date 14/09/2011
 */
class DLLExport NullCoordTransform : public Mantid::API::CoordTransform {
public:
  NullCoordTransform(size_t ndims = 3);
  virtual ~NullCoordTransform();
  std::string toXMLString() const;
  std::string id() const;
  void apply(const Mantid::coord_t *inputVector,
             Mantid::coord_t *outVector) const;
  virtual CoordTransform *clone() const;

private:
  /// Number of dimensions.
  size_t m_ndims;
};
}
}

#endif

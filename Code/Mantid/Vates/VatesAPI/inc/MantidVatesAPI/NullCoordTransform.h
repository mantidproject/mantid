#ifndef MANTID_VATES_API_NULL_COORD_TRANSFORM_H
#define MANTID_VATES_API_NULL_COORD_TRANSFORM_H

#include "MantidAPI/CoordTransform.h"

namespace Mantid
{
  namespace VATES
  {

  /** NullCoordTransform : A transform that sets the outVector to have the same values as the inputVector. 
   * Therefore has no-effect, for where transforms are not required.
   * @author Owen Arnold
   * @date 14/09/2011
   */
    class DLLExport NullCoordTransform : public Mantid::API::CoordTransform
    {
    public:

      NullCoordTransform(size_t ndims=3);
      virtual ~NullCoordTransform();
      std::string toXMLString() const;
      void apply(const Mantid::coord_t * inputVector, Mantid::coord_t * outVector) const;
      virtual CoordTransform * clone() const;

    private:
      /// Number of dimensions.
      size_t m_ndims;

    };
  }
}

#endif

#include "MantidVatesAPI/NullCoordTransform.h"

namespace Mantid
{
  namespace VATES
  {

    /** Constructor
    @param ndims : Number of dimensions
    */
    NullCoordTransform::NullCoordTransform(size_t ndims) : Mantid::API::CoordTransform(3, 3), m_ndims(ndims)
    {
    }

    /// Destructor
    NullCoordTransform::~NullCoordTransform()
    {
    }

    /** Serialize to a string.
    @throw runtime_error if used.
    */
    std::string NullCoordTransform::toXMLString() const 
    { 
      throw std::runtime_error("Not Implemented");
    }

    /**
    Apply the transformation.
    @param inputVector : pointer to the input vector
    @param outVector : pointer to the output vector.
    */
    void NullCoordTransform::apply(const Mantid::coord_t * inputVector, Mantid::coord_t * outVector) const
    {
      for(size_t i = 0; i < m_ndims; i++)
      {
        outVector[i] = inputVector[i];
      }
    }


  }
}
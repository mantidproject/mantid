// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/NullCoordTransform.h"
#include "MantidAPI/CoordTransform.h"

namespace Mantid::API {

/** Constructor
@param ndims : Number of dimensions
*/
NullCoordTransform::NullCoordTransform(size_t ndims) : Mantid::API::CoordTransform(ndims, ndims), m_ndims(ndims) {}

CoordTransform *NullCoordTransform::clone() const { return new NullCoordTransform(m_ndims); }

/** Serialize to a string.
@throw runtime_error if used.
*/
std::string NullCoordTransform::toXMLString() const { throw std::runtime_error("Not Implemented"); }

/**
 * Coordinate transform id
 * @return the type of coordinate transform
 */
std::string NullCoordTransform::id() const { return "NullCoordTransform"; }

/**
Apply the transformation.
@param inputVector : pointer to the input vector
@param outVector : pointer to the output vector.
*/
void NullCoordTransform::apply(const Mantid::coord_t *inputVector, Mantid::coord_t *outVector) const {
  for (size_t i = 0; i < m_ndims; i++) {
    outVector[i] = inputVector[i];
  }
}
} // namespace Mantid::API

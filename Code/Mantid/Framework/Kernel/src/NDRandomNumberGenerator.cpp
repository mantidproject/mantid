//
// Includes
//
#include "MantidKernel/NDRandomNumberGenerator.h"
#include <cassert>

namespace Mantid {
namespace Kernel {
/**
 * Construct an object
 */
NDRandomNumberGenerator::NDRandomNumberGenerator(const unsigned int ndims)
    : m_ndims(ndims), m_nextPoint(ndims, 0.0) {}

/**
 *  Generate the next set of values that form a point in ND space
 *  @returns A vector containing the ND random numbers
 */
const std::vector<double> &NDRandomNumberGenerator::nextPoint() {
  this->generateNextPoint();
  return m_nextPoint;
}

/**
 * Cache a value for a given dimension index, i.e. 0->ND-1
 * @param index :: An index for the dimension of the value generated
 * @param value :: value to be cached
 *
 */
void NDRandomNumberGenerator::cacheGeneratedValue(const size_t index,
                                                  const double value) {
  assert(index < m_ndims);
  m_nextPoint[index] = value;
}

/**
 * Cache the next point. Should be called by the concrete implementation at the
 * end of generateNextPoint
 * Avoids returning the vectors by value and copying.
 * @param nextPoint :: A vector containing the new set of points
 */
void
NDRandomNumberGenerator::cacheNextPoint(const std::vector<double> &nextPoint) {
  assert(nextPoint.size() == m_ndims);
  m_nextPoint = nextPoint;
}
}
}

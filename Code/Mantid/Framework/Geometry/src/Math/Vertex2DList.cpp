//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Math/Vertex2DList.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace Geometry {
using Kernel::V2D;

//--------------------------------------------------------------------------
// Public methods
//--------------------------------------------------------------------------

/**
 * Operator access, non-const version
 * @param index :: The vertex index to return
 * @returns The vertex at the given index
 * @throws Exception::IndexError if the index does not exist
 */
Kernel::V2D &Vertex2DList::operator[](const size_t index) {
  // Same code as const-version, so use it to avoid code duplication
  // First get a const this object with static_cast and then const_cast
  // away the const on the returned object
  return const_cast<V2D &>(static_cast<const Vertex2DList &>(*this)[index]);
}

/**
 * Operator access, const version
 * @param index :: The vertex index to return
 * @returns The vertex at the given index
 * @throws Exception::IndexError if the index does not exist
 */
const Kernel::V2D &Vertex2DList::operator[](const size_t index) const {
  if (index < m_vertices.size())
    return m_vertices[index];
  throw Kernel::Exception::IndexError(index, m_vertices.size(),
                                      "Vertex2DList::operator[]");
}

/**
 * Access the first element
 * @returns A const reference to the first element
 * @throws Exception::IndexError if the list is empty
 */
const Kernel::V2D &Vertex2DList::front() const { return this->operator[](0); }

/**
 * Access the last element
 * @returns A const reference to the first element
 * @throws Exception::IndexError if the list is empty
 */
const Kernel::V2D &Vertex2DList::back() const {
  return this->operator[](this->size() - 1);
}

/**
 * First searches for the point io the list, if it exists the index
 * of that point is returned. If it does not exist the point is appended
 * to the end.
 * @param point :: A new point to add
 * @returns True if the addition was successful, false otherwise
 */
unsigned int Vertex2DList::insert(const Kernel::V2D &point) {
  int index = indexOf(point);
  if (index < 0) {
    index = static_cast<unsigned int>(this->size());
    m_vertices.push_back(point);
  }
  return static_cast<unsigned int>(index);
}

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/**
 * Index of a point
 * @param point :: The point to check
 * @returns The index value of the point in the list of -1 if it is not in
 * the list
 */
int Vertex2DList::indexOf(const Kernel::V2D &point) const {
  size_t index(0);
  const size_t npoints(this->size());
  for (; index < npoints; ++index) {
    if (m_vertices[index] == point)
      return static_cast<int>(index);
  }
  return -1;
}

} // namespace Mantid
} // namespace Geometry

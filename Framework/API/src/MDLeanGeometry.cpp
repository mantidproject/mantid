#include "MantidAPI/MDLeanGeometry.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {

MDLeanGeometry::MDLeanGeometry() : m_dimensions(), m_basisVectors() {}

MDLeanGeometry::MDLeanGeometry(const MDLeanGeometry &other)
    : m_dimensions(), m_basisVectors(other.m_basisVectors) {
  // Perform a deep copy of the dimensions
  std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions;
  for (size_t d = 0; d < other.getNumDims(); d++) {
    // Copy the dimension
    auto dim = boost::make_shared<Mantid::Geometry::MDHistoDimension>(
        other.getDimension(d).get());
    dimensions.push_back(dim);
  }

  this->initGeometry(dimensions);
}

MDLeanGeometry::~MDLeanGeometry() { m_dimensions.clear(); }

//----------------------------------------------------------------------------------------------
/**
 * Initialize the geometry
 *
 * @param dimensions :: vector of IMDDimension objects, in the order
 * of X, Y, Z, t, etc.
 */
void MDLeanGeometry::initGeometry(
    const std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) {
  // Copy the dimensions array
  m_dimensions = dimensions;
  // Make sure the basis vectors are big enough
  m_basisVectors.resize(m_dimensions.size(), Mantid::Kernel::VMD());
}

// --------------------------------------------------------------------------------------------
/**
 * @return the number of dimensions in this workspace
 */
size_t MDLeanGeometry::getNumDims() const { return m_dimensions.size(); }

// --------------------------------------------------------------------------------------------
/**
 * Get a dimension
 *
 * @param index :: which dimension
 * @return the dimension at that index
 */
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDLeanGeometry::getDimension(size_t index) const {
  if (index >= m_dimensions.size())
    throw std::runtime_error(
        "Workspace does not have a dimension at that index.");
  return m_dimensions[index];
}

// --------------------------------------------------------------------------------------------
/**
 * Get a dimension
 *
 * @param id :: string ID of the dimension
 * @return the dimension with the specified id string.
 */
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDLeanGeometry::getDimensionWithId(std::string id) const {
  auto dimension = std::find_if(
      m_dimensions.begin(), m_dimensions.end(),
      [&id](
          const boost::shared_ptr<const Mantid::Geometry::IMDDimension> &dim) {
        return dim->getDimensionId() == id;
      });
  if (dimension != m_dimensions.end())
    return *dimension;
  else
    throw std::invalid_argument("Dimension tagged " + id +
                                " was not found in the Workspace");
}

// --------------------------------------------------------------------------------------------
/**
 * Get non-collapsed dimensions
 *
 * @return vector of collapsed dimensions in the workspace geometry.
 */
Mantid::Geometry::VecIMDDimension_const_sptr
MDLeanGeometry::getNonIntegratedDimensions() const {
  using namespace Mantid::Geometry;
  VecIMDDimension_const_sptr vecCollapsedDimensions;
  for (auto current : this->m_dimensions) {
    if (!current->getIsIntegrated()) {
      vecCollapsedDimensions.push_back(current);
    }
  }
  return vecCollapsedDimensions;
}

//-----------------------------------------------------------------------------------------------
/**
 * @return a vector with the size of the smallest bin in each dimension
 */
std::vector<coord_t> MDLeanGeometry::estimateResolution() const {
  std::vector<coord_t> out;
  for (size_t d = 0; d < this->getNumDims(); d++)
    out.push_back(this->getDimension(d)->getBinWidth());
  return out;
}

//-----------------------------------------------------------------------------------------------
/**
 * Get the index of the dimension that matches the name given
 *
 * @param name :: name of the m_dimensions
 * @return the index (size_t)
 * @throw runtime_error if it cannot be found.
 */
size_t MDLeanGeometry::getDimensionIndexByName(const std::string &name) const {
  for (size_t d = 0; d < m_dimensions.size(); d++)
    if (m_dimensions[d]->getName() == name)
      return d;
  throw std::runtime_error("Dimension named '" + name +
                           "' was not found in the IMDWorkspace.");
}

//-----------------------------------------------------------------------------------------------
/**
 * Get the index of the dimension that matches the ID given
 *
 * @param id :: id string of the dimension
 * @return the index (size_t)
 * @throw runtime_error if it cannot be found.
 */
size_t MDLeanGeometry::getDimensionIndexById(const std::string &id) const {
  for (size_t d = 0; d < m_dimensions.size(); d++)
    if (m_dimensions[d]->getDimensionId() == id)
      return d;
  throw std::runtime_error("Dimension with id '" + id +
                           "' was not found in the IMDWorkspace.");
}

// --------------------------------------------------------------------------------------------
/**
 * Add a dimension
 * @param dim :: shared pointer to the dimension object
 */
void MDLeanGeometry::addDimension(
    boost::shared_ptr<Mantid::Geometry::IMDDimension> dim) {
  m_dimensions.push_back(dim);
}

// --------------------------------------------------------------------------------------------
/**
 * Add a dimension
 *
 * @param dim :: bare pointer to the dimension object
 */
void MDLeanGeometry::addDimension(Mantid::Geometry::IMDDimension *dim) {
  m_dimensions.push_back(
      boost::shared_ptr<Mantid::Geometry::IMDDimension>(dim));
}

// --------------------------------------------------------------------------------------------
/**
 * Get the basis vector (in the original workspace) for a dimension of this
 * workspace.
 *
 * @param index :: which dimension
 * @return a vector, in the dimensions of the original workspace
 */
Mantid::Kernel::VMD &MDLeanGeometry::getBasisVector(size_t index) {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  return m_basisVectors[index];
}

/**
 * Get the basis vector (in the original workspace) for a dimension of this
 * workspace.
 *
 * @param index :: which dimension
 * @return a vector, in the dimensions of the original workspace
 */
const Mantid::Kernel::VMD &MDLeanGeometry::getBasisVector(size_t index) const {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  return m_basisVectors[index];
}

/**
 * Set the basis vector (in the original workspace) for a dimension of this
 * workspace.
 *
 * @param index :: which dimension
 * @param vec :: a vector, in the dimensions of the original workspace
 */
void MDLeanGeometry::setBasisVector(size_t index,
                                    const Mantid::Kernel::VMD &vec) {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  m_basisVectors[index] = vec;
}

/**
 * @return True ONLY if ALL the basis vectors have been normalized.
 */
bool MDLeanGeometry::allBasisNormalized() const {
  auto normalized = std::find_if(m_basisVectors.begin(), m_basisVectors.end(),
                                 [](const Mantid::Kernel::VMD &basisVector) {
                                   return basisVector.length() != 1.0;
                                 });
  return normalized == m_basisVectors.end();
}

} // namespace Mantid
} // namespace API

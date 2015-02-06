#include "MantidAPI/MDGeometry.h"
#include "MantidKernel/System.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDGeometry::MDGeometry()
    : m_originalWorkspaces(), m_transforms_FromOriginal(),
      m_transforms_ToOriginal(),
      m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
      m_observingDelete(false), m_Wtransf(3, 3, true) {}

//----------------------------------------------------------------------------------------------
/** Copy Constructor
 */
MDGeometry::MDGeometry(const MDGeometry &other)
    : m_originalWorkspaces(), m_origin(other.m_origin),
      m_transforms_FromOriginal(), m_transforms_ToOriginal(),
      m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
      m_observingDelete(false), m_Wtransf(other.m_Wtransf),
      m_basisVectors(other.m_basisVectors) {
  // Perform a deep copy of the dimensions
  std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions;
  for (size_t d = 0; d < other.getNumDims(); d++) {
    // Copy the dimension
    MDHistoDimension_sptr dim(
        new MDHistoDimension(other.getDimension(d).get()));
    dimensions.push_back(dim);
  }
  this->initGeometry(dimensions);

  // Perform a deep copy of the coordinate transformations
  std::vector<CoordTransform *>::const_iterator it;
  for (it = other.m_transforms_FromOriginal.begin();
       it != other.m_transforms_FromOriginal.end(); ++it) {
    if (*it)
      m_transforms_FromOriginal.push_back((*it)->clone());
    else
      m_transforms_FromOriginal.push_back(NULL);
  }

  for (it = other.m_transforms_ToOriginal.begin();
       it != other.m_transforms_ToOriginal.end(); ++it) {
    if (*it)
      m_transforms_ToOriginal.push_back((*it)->clone());
    else
      m_transforms_ToOriginal.push_back(NULL);
  }

  // Copy the references to the original workspaces
  // This will also set up the delete observer to listen to those workspaces
  // being deleted.
  for (size_t i = 0; i < other.m_originalWorkspaces.size(); i++)
    this->setOriginalWorkspace(other.m_originalWorkspaces[i], i);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDGeometry::~MDGeometry() {
  for (size_t i = 0; i < m_transforms_FromOriginal.size(); i++)
    delete m_transforms_FromOriginal[i];
  for (size_t i = 0; i < m_transforms_ToOriginal.size(); i++)
    delete m_transforms_ToOriginal[i];
  if (m_observingDelete) {
    // Stop watching once object is deleted
    API::AnalysisDataService::Instance().notificationCenter.removeObserver(
        m_delete_observer);
  }
  m_dimensions.clear();
}

//----------------------------------------------------------------------------------------------
/** Initialize the geometry
 *
 * @param dimensions :: vector of IMDDimension objects, in the order of X, Y, Z,
 *t, etc.
 */
void MDGeometry::initGeometry(
    std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) {
  if (dimensions.size() == 0)
    throw std::invalid_argument(
        "MDGeometry::initGeometry() 0 valid dimensions were given!");

  // Copy the dimensions array
  m_dimensions = dimensions;
  // Make sure the basis vectors are big enough
  m_basisVectors.resize(m_dimensions.size(), Mantid::Kernel::VMD());
}

// --------------------------------------------------------------------------------------------
/** @return the number of dimensions in this workspace */
size_t MDGeometry::getNumDims() const { return m_dimensions.size(); }

// --------------------------------------------------------------------------------------------
/** Get a dimension
 * @param index :: which dimension
 * @return the dimension at that index
 */
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getDimension(size_t index) const {
  if (index >= m_dimensions.size())
    throw std::runtime_error(
        "Workspace does not have a dimension at that index.");
  return m_dimensions[index];
}

// --------------------------------------------------------------------------------------------
/** Get a dimension
 * @param id :: string ID of the dimension
 * @return the dimension with the specified id string.
 */
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getDimensionWithId(std::string id) const {
  for (size_t i = 0; i < m_dimensions.size(); ++i)
    if (m_dimensions[i]->getDimensionId() == id)
      return m_dimensions[i];
  throw std::invalid_argument("Dimension tagged " + id +
                              " was not found in the Workspace");
}

// --------------------------------------------------------------------------------------------
/** Get non-collapsed dimensions
@return vector of collapsed dimensions in the workspace geometry.
*/
Mantid::Geometry::VecIMDDimension_const_sptr
MDGeometry::getNonIntegratedDimensions() const {
  using namespace Mantid::Geometry;
  VecIMDDimension_const_sptr vecCollapsedDimensions;
  std::vector<Mantid::Geometry::IMDDimension_sptr>::const_iterator it =
      this->m_dimensions.begin();
  for (; it != this->m_dimensions.end(); ++it) {
    IMDDimension_sptr current = (*it);
    if (!current->getIsIntegrated()) {
      vecCollapsedDimensions.push_back(current);
    }
  }
  return vecCollapsedDimensions;
}

//-----------------------------------------------------------------------------------------------
/** @return a vector with the size of the smallest bin in each dimension */
std::vector<coord_t> MDGeometry::estimateResolution() const {
  std::vector<coord_t> out;
  for (size_t d = 0; d < this->getNumDims(); d++)
    out.push_back(this->getDimension(d)->getBinWidth());
  return out;
}

//-----------------------------------------------------------------------------------------------
/** Get the index of the dimension that matches the name given
 *
 * @param name :: name of the m_dimensions
 * @return the index (size_t)
 * @throw runtime_error if it cannot be found.
 */
size_t MDGeometry::getDimensionIndexByName(const std::string &name) const {
  for (size_t d = 0; d < m_dimensions.size(); d++)
    if (m_dimensions[d]->getName() == name)
      return d;
  throw std::runtime_error("Dimension named '" + name +
                           "' was not found in the IMDWorkspace.");
}

//-----------------------------------------------------------------------------------------------
/** Get the index of the dimension that matches the ID given
 *
 * @param id :: id string of the dimension
 * @return the index (size_t)
 * @throw runtime_error if it cannot be found.
 */
size_t MDGeometry::getDimensionIndexById(const std::string &id) const {
  for (size_t d = 0; d < m_dimensions.size(); d++)
    if (m_dimensions[d]->getDimensionId() == id)
      return d;
  throw std::runtime_error("Dimension with id '" + id +
                           "' was not found in the IMDWorkspace.");
}

// --------------------------------------------------------------------------------------------
/** Add a dimension
 * @param dim :: shared pointer to the dimension object   */
void MDGeometry::addDimension(
    boost::shared_ptr<Mantid::Geometry::IMDDimension> dim) {
  m_dimensions.push_back(dim);
}

// --------------------------------------------------------------------------------------------
/** Add a dimension
 * @param dim :: bare pointer to the dimension object   */
void MDGeometry::addDimension(Mantid::Geometry::IMDDimension *dim) {
  m_dimensions.push_back(
      boost::shared_ptr<Mantid::Geometry::IMDDimension>(dim));
}

// --------------------------------------------------------------------------------------------
/// Get the x-dimension mapping.
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getXDimension() const {
  if (this->getNumDims() < 1)
    throw std::runtime_error("Workspace does not have any dimensions!");
  return this->getDimension(0);
}

/// Get the y-dimension mapping.
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getYDimension() const {
  if (this->getNumDims() < 2)
    throw std::runtime_error("Workspace does not have a Y dimension.");
  return this->getDimension(1);
}

/// Get the z-dimension mapping.
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getZDimension() const {
  if (this->getNumDims() < 3)
    throw std::runtime_error("Workspace does not have a X dimension.");
  return this->getDimension(2);
}

/// Get the t-dimension mapping.
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getTDimension() const {
  if (this->getNumDims() < 4)
    throw std::runtime_error("Workspace does not have a T dimension.");
  return this->getDimension(3);
}

// --------------------------------------------------------------------------------------------
/** Get the basis vector (in the original workspace) for a dimension of this
 * workspace.
 * @param index :: which dimension
 * @return a vector, in the dimensions of the original workspace
 */
Mantid::Kernel::VMD &MDGeometry::getBasisVector(size_t index) {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  return m_basisVectors[index];
}

/** Get the basis vector (in the original workspace) for a dimension of this
 * workspace.
 * @param index :: which dimension
 * @return a vector, in the dimensions of the original workspace
 */
const Mantid::Kernel::VMD &MDGeometry::getBasisVector(size_t index) const {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  return m_basisVectors[index];
}

/** Set the basis vector (in the original workspace) for a dimension of this
 * workspace.
 * @param index :: which dimension
 * @param vec :: a vector, in the dimensions of the original workspace
 */
void MDGeometry::setBasisVector(size_t index, const Mantid::Kernel::VMD &vec) {
  if (index >= m_basisVectors.size())
    throw std::invalid_argument("getBasisVector(): invalid index");
  m_basisVectors[index] = vec;
}

//---------------------------------------------------------------------------------------------------
/// @return true if the geometry is defined relative to another workspace.
/// @param index :: index into the vector of original workspaces
bool MDGeometry::hasOriginalWorkspace(size_t index) const {
  if (index >= m_originalWorkspaces.size())
    return false;
  return bool(m_originalWorkspaces[index]);
}

/// @return the number of original workspaces attached to this one
size_t MDGeometry::numOriginalWorkspaces() const {
  return m_originalWorkspaces.size();
}

//---------------------------------------------------------------------------------------------------
/** Get the "original" workspace (the workspace that was the source for a binned
 *MDWorkspace).
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @return the original workspace to which the basis vectors relate
 * @param index :: index into the vector of original workspaces.
 */
boost::shared_ptr<Workspace>
MDGeometry::getOriginalWorkspace(size_t index) const {
  if (index >= m_originalWorkspaces.size())
    throw std::runtime_error(
        "MDGeometry::getOriginalWorkspace() invalid index.");
  return m_originalWorkspaces[index];
}

//---------------------------------------------------------------------------------------------------
/** Set the "original" workspace (the workspace that was the source for a binned
 *MDWorkspace).
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @param ws :: original workspace shared pointer
 * @param index :: index into the vector of original workspaces.
 */
void MDGeometry::setOriginalWorkspace(boost::shared_ptr<Workspace> ws,
                                      size_t index) {
  if (index >= m_originalWorkspaces.size())
    m_originalWorkspaces.resize(index + 1);
  m_originalWorkspaces[index] = ws;
  // Watch for workspace deletions
  if (!m_observingDelete) {
    API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_delete_observer);
    m_observingDelete = true;
  }
}

//---------------------------------------------------------------------------------------------------
/** Transform the dimensions contained in this geometry
 * x' = x*scaling + offset
 *
 * This clears any original workspace or coordinate transformation
 * contained.
 *
 * NOTE! This does not modify any other underlying data. Call the TransformMD
 * algorithm to perform a full transform.
 *
 * @param scaling :: multiply each coordinate by this value.
 * @param offset :: after multiplying, add this offset.
 */
void MDGeometry::transformDimensions(std::vector<double> &scaling,
                                     std::vector<double> &offset) {
  if (scaling.size() != m_dimensions.size())
    throw std::invalid_argument("MDGeometry::transformDimensions(): "
                                "scaling.size() must be equal to number of "
                                "dimensions.");
  if (offset.size() != m_dimensions.size())
    throw std::invalid_argument("MDGeometry::transformDimensions(): "
                                "offset.size() must be equal to number of "
                                "dimensions.");
  for (size_t d = 0; d < m_dimensions.size(); d++) {
    IMDDimension_sptr dim = m_dimensions[d];
    coord_t min = (dim->getMinimum() * static_cast<coord_t>(scaling[d])) +
                  static_cast<coord_t>(offset[d]);
    coord_t max = (dim->getMaximum() * static_cast<coord_t>(scaling[d])) +
                  static_cast<coord_t>(offset[d]);
    dim->setRange(dim->getNBins(), min, max);
  }
  // Clear the original workspace
  setOriginalWorkspace(boost::shared_ptr<Workspace>());
  setTransformFromOriginal(NULL);
  setTransformToOriginal(NULL);
}

//---------------------------------------------------------------------------------------------------
/** Function called when observer objects receives a notification that
 * a workspace has been deleted.
 *
 * This checks if the "original workspace" in this object is being deleted,
 * and removes the reference to it to allow it to be destructed properly.
 *
 * @param notice :: notification of workspace deletion
 */
void MDGeometry::deleteNotificationReceived(
    Mantid::API::WorkspacePreDeleteNotification_ptr notice) {
  for (size_t i = 0; i < m_originalWorkspaces.size(); i++) {
    Workspace_sptr original = m_originalWorkspaces[i];
    if (original) {
      // Compare the pointer being deleted to the one stored as the original.
      Workspace_sptr deleted = notice->object();
      if (original == deleted) {
        // Clear the reference
        m_originalWorkspaces[i].reset();
      }
    }
  }
}

//---------------------------------------------------------------------------------------------------
/** Get the Coordinate Transformation that goes from the original workspace
 * to this workspace's coordinates.
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @return CoordTransform pointer
 * @param index :: index into the array of original workspaces
 */
Mantid::API::CoordTransform *
MDGeometry::getTransformFromOriginal(size_t index) const {
  if (index >= m_transforms_FromOriginal.size())
    throw std::runtime_error(
        "MDGeometry::getTransformFromOriginal(): invalid index.");
  return m_transforms_FromOriginal[index];
}

//---------------------------------------------------------------------------------------------------
/** Sets the Coordinate Transformation that goes from the original workspace
 * to this workspace's coordinates.
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @param transform :: CoordTransform pointer (this assumes pointer ownership)
 * @param index :: index into the array of original workspaces
 */
void
MDGeometry::setTransformFromOriginal(Mantid::API::CoordTransform *transform,
                                     size_t index) {
  if (index >= m_transforms_FromOriginal.size())
    m_transforms_FromOriginal.resize(index + 1);
  if (m_transforms_FromOriginal[index])
    delete m_transforms_FromOriginal[index];
  m_transforms_FromOriginal[index] = transform;
}

//---------------------------------------------------------------------------------------------------
/** Get the Coordinate Transformation that goes from THIS workspace's
 *coordinates
 * to the ORIGINAL workspace's coordinates
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @return CoordTransform pointer
 * @param index :: index into the array of original workspaces
 */
Mantid::API::CoordTransform *
MDGeometry::getTransformToOriginal(size_t index) const {
  if (index >= m_transforms_ToOriginal.size())
    throw std::runtime_error(
        "MDGeometry::getTransformFromOriginal(): invalid index.");
  return m_transforms_ToOriginal[index];
}

//---------------------------------------------------------------------------------------------------
/** Sets the Coordinate Transformation that goes from THIS workspace's
 *coordinates
 * to the ORIGINAL workspace's coordinates
 *
 *  In the case of a chain of workspaces: A->binned to B->binned to C:
 *    Index 0 = the workspace that was binned, e.g. "A"
 *    Index 1 = the intermediate workspace, e.g. "B"
 *
 * @param transform :: CoordTransform pointer (this assumes pointer ownership)
 * @param index :: index into the array of original workspaces
 */
void MDGeometry::setTransformToOriginal(Mantid::API::CoordTransform *transform,
                                        size_t index) {
  if (index >= m_transforms_ToOriginal.size())
    m_transforms_ToOriginal.resize(index + 1);
  if (m_transforms_ToOriginal[index])
    delete m_transforms_ToOriginal[index];
  m_transforms_ToOriginal[index] = transform;
}

//---------------------------------------------------------------------------------------------------
/** @return a XML representation of the geometry of the workspace */
std::string MDGeometry::getGeometryXML() const {
  using Mantid::Geometry::MDGeometryBuilderXML;
  using Mantid::Geometry::NoDimensionPolicy;
  MDGeometryBuilderXML<NoDimensionPolicy> xmlBuilder;
  // Add all dimensions.
  const size_t nDimensions = this->getNumDims();
  for (size_t i = 0; i < nDimensions; i++) {
    xmlBuilder.addOrdinaryDimension(this->getDimension(i));
  }
  // Add mapping dimensions
  if (nDimensions > 0) {
    xmlBuilder.addXDimension(this->getXDimension());
  }
  if (nDimensions > 1) {
    xmlBuilder.addYDimension(this->getYDimension());
  }
  if (nDimensions > 2) {
    xmlBuilder.addZDimension(this->getZDimension());
  }
  if (nDimensions > 3) {
    xmlBuilder.addTDimension(this->getTDimension());
  }
  // Create the xml.
  return xmlBuilder.create();
}

/**
 * Get the number of transforms defined to the original coordinate system.
 * @return The number of transforms.
 */
size_t MDGeometry::getNumberTransformsToOriginal() const {
  return m_transforms_ToOriginal.size();
}

/**
 * Get the number of transforms defined from the original coordinate system.
 * @return The number of transforms.
 */
size_t MDGeometry::getNumberTransformsFromOriginal() const {
  return m_transforms_FromOriginal.size();
}

} // namespace Mantid
} // namespace API

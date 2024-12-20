// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CoordTransform.h"

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <Poco/NObserver.h>
#include <memory>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid::API {

class MDGeometryNotificationHelper {
public:
  explicit MDGeometryNotificationHelper(MDGeometry &parent)
      : m_parent(parent), m_delete_observer(*this, &MDGeometryNotificationHelper::deleteNotificationReceived),
        m_replace_observer(*this, &MDGeometryNotificationHelper::replaceNotificationReceived) {}

  ~MDGeometryNotificationHelper() {
    if (m_observingDelete) {
      // Stop watching once object is deleted
      API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
    }
    if (m_observingReplace) {
      API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replace_observer);
    }
  }

  void watchForWorkspaceDeletions() {
    if (!m_observingDelete) {
      API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
      m_observingDelete = true;
    }
  }

  void watchForWorkspaceReplace() {
    if (!m_observingReplace) {
      API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replace_observer);
      m_observingReplace = true;
    }
  }

  void deleteNotificationReceived(Mantid::API::WorkspacePreDeleteNotification_ptr notice) {
    m_parent.deleteNotificationReceived(notice->object());
  }

  void replaceNotificationReceived(Mantid::API::WorkspaceBeforeReplaceNotification_ptr notice) {
    m_parent.replaceNotificationReceived(notice->object());
  }

private:
  MDGeometry &m_parent;

  /// Poco delete notification observer object
  Poco::NObserver<MDGeometryNotificationHelper, WorkspacePreDeleteNotification> m_delete_observer;
  Poco::NObserver<MDGeometryNotificationHelper, WorkspaceBeforeReplaceNotification> m_replace_observer;

  /// Set to True when the m_delete_observer is observing workspace deletions.
  bool m_observingDelete{false};
  bool m_observingReplace{false};
};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDGeometry::MDGeometry()
    : m_dimensions(), m_originalWorkspaces(), m_origin(), m_transforms_FromOriginal(), m_transforms_ToOriginal(),
      m_notificationHelper(std::make_unique<MDGeometryNotificationHelper>(*this)), m_Wtransf(3, 3, true),
      m_basisVectors() {}

//----------------------------------------------------------------------------------------------
/** Copy Constructor
 */
MDGeometry::MDGeometry(const MDGeometry &other) { *this = other; }

MDGeometry &MDGeometry::operator=(const MDGeometry &other) {
  m_dimensions = std::vector<std::shared_ptr<IMDDimension>>{};
  m_originalWorkspaces = std::vector<std::shared_ptr<Workspace>>{};
  m_origin = other.m_origin;
  m_transforms_FromOriginal = std::vector<std::shared_ptr<const CoordTransform>>{};
  m_transforms_ToOriginal = std::vector<std::shared_ptr<const CoordTransform>>{};
  m_notificationHelper = std::make_unique<MDGeometryNotificationHelper>(*this);
  m_Wtransf = other.m_Wtransf;
  m_basisVectors = other.m_basisVectors;

  // Perform a deep copy of the dimensions
  std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions;
  for (size_t d = 0; d < other.getNumDims(); d++) {
    // Copy the dimension
    auto dim = std::make_shared<MDHistoDimension>(other.getDimension(d).get());
    dimensions.emplace_back(dim);
  }
  this->initGeometry(dimensions);

  // Perform a deep copy of the coordinate transformations
  std::vector<CoordTransform_const_sptr>::const_iterator it;
  for (it = other.m_transforms_FromOriginal.begin(); it != other.m_transforms_FromOriginal.end(); ++it) {
    if (*it)
      m_transforms_FromOriginal.emplace_back(CoordTransform_const_sptr((*it)->clone()));
    else
      m_transforms_FromOriginal.emplace_back(CoordTransform_const_sptr());
  }

  for (it = other.m_transforms_ToOriginal.begin(); it != other.m_transforms_ToOriginal.end(); ++it) {
    if (*it)
      m_transforms_ToOriginal.emplace_back(CoordTransform_const_sptr((*it)->clone()));
    else
      m_transforms_ToOriginal.emplace_back(CoordTransform_const_sptr());
  }

  // Copy the references to the original workspaces
  // This will also set up the delete observer to listen to those workspaces
  // being deleted.
  for (size_t i = 0; i < other.m_originalWorkspaces.size(); i++)
    this->setOriginalWorkspace(other.m_originalWorkspaces[i], i);

  return *this;
}

/**
 * Clear all transforms to and from original workspaces.
 */
void MDGeometry::clearTransforms() {
  m_transforms_ToOriginal.clear();
  m_transforms_FromOriginal.clear();
}

/**
 * Clear the original workspaces
 */
void MDGeometry::clearOriginalWorkspaces() { m_originalWorkspaces.clear(); }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDGeometry::~MDGeometry() { m_dimensions.clear(); }

//----------------------------------------------------------------------------------------------
/** Initialize the geometry
 *
 * @param dimensions :: vector of IMDDimension objects, in the order of X, Y, Z,
 *t, etc.
 */
void MDGeometry::initGeometry(const std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) {
  // Copy the dimensions array
  m_dimensions = dimensions;
  // Make sure the basis vectors are big enough
  m_basisVectors.resize(m_dimensions.size(), Mantid::Kernel::VMD());
}

// --------------------------------------------------------------------------------------------
/** @return the number of dimensions in this workspace */
size_t MDGeometry::getNumDims() const { return m_dimensions.size(); }

/** @return the number of non-integrated dimensions in this workspace */
size_t MDGeometry::getNumNonIntegratedDims() const {
  return std::count_if(m_dimensions.cbegin(), m_dimensions.cend(),
                       [](const auto &dimension) { return !dimension->getIsIntegrated(); });
}

// --------------------------------------------------------------------------------------------
/** Get a dimension
 * @param index :: which dimension
 * @return the dimension at that index
 */
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimension(size_t index) const {
  if (index >= m_dimensions.size())
    throw std::runtime_error("Workspace does not have a dimension at that index.");
  return m_dimensions[index];
}

// --------------------------------------------------------------------------------------------
/** Get a dimension
 * @param id :: string ID of the dimension
 * @return the dimension with the specified id string.
 */
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimensionWithId(std::string id) const {
  auto dimension = std::find_if(
      m_dimensions.begin(), m_dimensions.end(),
      [&id](const std::shared_ptr<const Mantid::Geometry::IMDDimension> &dim) { return dim->getDimensionId() == id; });
  if (dimension != m_dimensions.end())
    return *dimension;
  else
    throw std::invalid_argument("Dimension tagged " + id + " was not found in the Workspace");
}

// --------------------------------------------------------------------------------------------
/** Get non-collapsed dimensions
@return vector of collapsed dimensions in the workspace geometry.
*/
Mantid::Geometry::VecIMDDimension_const_sptr MDGeometry::getNonIntegratedDimensions() const {
  using namespace Mantid::Geometry;
  VecIMDDimension_const_sptr vecCollapsedDimensions;
  std::copy_if(m_dimensions.cbegin(), m_dimensions.cend(), std::back_inserter(vecCollapsedDimensions),
               [](const auto &dimension) { return !dimension->getIsIntegrated(); });
  return vecCollapsedDimensions;
}

//-----------------------------------------------------------------------------------------------
/** @return a vector with the size of the smallest bin in each dimension */
std::vector<coord_t> MDGeometry::estimateResolution() const {
  std::vector<coord_t> out;
  for (size_t d = 0; d < this->getNumDims(); d++)
    out.emplace_back(this->getDimension(d)->getBinWidth());
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
  throw std::runtime_error("Dimension named '" + name + "' was not found in the IMDWorkspace.");
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
  throw std::runtime_error("Dimension with id '" + id + "' was not found in the IMDWorkspace.");
}

// --------------------------------------------------------------------------------------------
/** Add a dimension
 * @param dim :: shared pointer to the dimension object   */
void MDGeometry::addDimension(const std::shared_ptr<Mantid::Geometry::IMDDimension> &dim) {
  m_dimensions.emplace_back(dim);
}

// --------------------------------------------------------------------------------------------
/** Add a dimension
 * @param dim :: bare pointer to the dimension object   */
void MDGeometry::addDimension(Mantid::Geometry::IMDDimension *dim) {
  m_dimensions.emplace_back(std::shared_ptr<Mantid::Geometry::IMDDimension>(dim));
}

// --------------------------------------------------------------------------------------------
/// Get the x-dimension mapping.
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getXDimension() const {
  if (this->getNumDims() < 1)
    throw std::runtime_error("Workspace does not have any dimensions!");
  return this->getDimension(0);
}

/// Get the y-dimension mapping.
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getYDimension() const {
  if (this->getNumDims() < 2)
    throw std::runtime_error("Workspace does not have a Y dimension.");
  return this->getDimension(1);
}

/// Get the z-dimension mapping.
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getZDimension() const {
  if (this->getNumDims() < 3)
    throw std::runtime_error("Workspace does not have a Z dimension.");
  return this->getDimension(2);
}

/// Get the t-dimension mapping.
std::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getTDimension() const {
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

/**
 * @return True ONLY if ALL the basis vectors have been normalized.
 */
bool MDGeometry::allBasisNormalized() const {
  auto normalized = std::find_if(m_basisVectors.begin(), m_basisVectors.end(),
                                 [](const Mantid::Kernel::VMD &basisVector) { return basisVector.length() != 1.0; });
  return normalized == m_basisVectors.end();
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
size_t MDGeometry::numOriginalWorkspaces() const { return m_originalWorkspaces.size(); }

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
std::shared_ptr<Workspace> MDGeometry::getOriginalWorkspace(size_t index) const {
  if (index >= m_originalWorkspaces.size())
    throw std::runtime_error("MDGeometry::getOriginalWorkspace() invalid index.");
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
void MDGeometry::setOriginalWorkspace(std::shared_ptr<Workspace> ws, size_t index) {
  if (index >= m_originalWorkspaces.size())
    m_originalWorkspaces.resize(index + 1);
  m_originalWorkspaces[index] = std::move(ws);
  m_notificationHelper->watchForWorkspaceDeletions();
  m_notificationHelper->watchForWorkspaceReplace();
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
void MDGeometry::transformDimensions(std::vector<double> &scaling, std::vector<double> &offset) {
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
    coord_t min = (dim->getMinimum() * static_cast<coord_t>(scaling[d])) + static_cast<coord_t>(offset[d]);
    coord_t max = (dim->getMaximum() * static_cast<coord_t>(scaling[d])) + static_cast<coord_t>(offset[d]);
    if (min < max)
      dim->setRange(dim->getNBins(), min, max);
    else
      dim->setRange(dim->getNBins(), max, min);
  }
  // Clear the original workspace
  setOriginalWorkspace(std::shared_ptr<Workspace>());
  setTransformFromOriginal(nullptr);
  setTransformToOriginal(nullptr);
}

//---------------------------------------------------------------------------------------------------
/** Function called when observer objects receives a notification that
 * a workspace has been deleted.
 *
 * This checks if the "original workspace" in this object is being deleted,
 * and removes the reference to it to allow it to be destructed properly.
 *
 * @param deleted :: The deleted workspace
 */
void MDGeometry::deleteNotificationReceived(const std::shared_ptr<const Workspace> &deleted) {
  for (auto &original : m_originalWorkspaces) {
    if (original) {
      // Compare the pointer being deleted to the one stored as the original.
      if (original == deleted) {
        // Clear the reference
        original.reset();
      }
    }
  }
}

//---------------------------------------------------------------------------------------------------
/** Function called when observer objects receives a notification that
 * a workspace has been replaced.
 *
 * This checks if the "original workspace" in this object is being replaced,
 * and removes the reference to it to allow it to be destructed properly.
 *
 * @param replaced :: The replaced workspace
 */
void MDGeometry::replaceNotificationReceived(const std::shared_ptr<const Workspace> &replaced) {
  for (auto &original : m_originalWorkspaces) {
    if (original) {
      // Compare the pointer being replaced to the one stored as the original.
      if (original == replaced) {
        // Clear the reference
        original.reset();
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
Mantid::API::CoordTransform const *MDGeometry::getTransformFromOriginal(size_t index) const {
  if (index >= m_transforms_FromOriginal.size())
    throw std::runtime_error("MDGeometry::getTransformFromOriginal(): invalid index.");
  return m_transforms_FromOriginal[index].get();
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
void MDGeometry::setTransformFromOriginal(Mantid::API::CoordTransform *transform, size_t index) {
  if (index >= m_transforms_FromOriginal.size()) {
    m_transforms_FromOriginal.resize(index + 1);
  }
  m_transforms_FromOriginal[index] = CoordTransform_const_sptr(transform);
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
Mantid::API::CoordTransform const *MDGeometry::getTransformToOriginal(size_t index) const {
  if (index >= m_transforms_ToOriginal.size())
    throw std::runtime_error("MDGeometry::getTransformFromOriginal(): invalid index.");
  return m_transforms_ToOriginal[index].get();
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
void MDGeometry::setTransformToOriginal(Mantid::API::CoordTransform *transform, size_t index) {
  if (index >= m_transforms_ToOriginal.size()) {
    m_transforms_ToOriginal.resize(index + 1);
  }
  m_transforms_ToOriginal[index] = CoordTransform_const_sptr(transform);
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
size_t MDGeometry::getNumberTransformsToOriginal() const { return m_transforms_ToOriginal.size(); }

/**
 * Get the number of transforms defined from the original coordinate system.
 * @return The number of transforms.
 */
size_t MDGeometry::getNumberTransformsFromOriginal() const { return m_transforms_FromOriginal.size(); }

} // namespace Mantid::API

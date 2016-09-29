#include "MantidAPI/MDGeometry.h"
#include "MantidKernel/System.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDGeometry::MDGeometry()
    : m_originalWorkspaces(), m_origin(), m_transforms_FromOriginal(),
      m_transforms_ToOriginal(),
      m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
      m_observingDelete(false), m_Wtransf(3, 3, true) {}

//----------------------------------------------------------------------------------------------
/** Copy Constructor
 */
MDGeometry::MDGeometry(const MDGeometry &other)
    : MDLeanGeometry(other), m_originalWorkspaces(), m_origin(other.m_origin),
      m_transforms_FromOriginal(), m_transforms_ToOriginal(),
      m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
      m_observingDelete(false), m_Wtransf(other.m_Wtransf) {

  // Perform a deep copy of the coordinate transformations
  std::vector<CoordTransform_const_sptr>::const_iterator it;
  for (it = other.m_transforms_FromOriginal.begin();
       it != other.m_transforms_FromOriginal.end(); ++it) {
    if (*it)
      m_transforms_FromOriginal.push_back(
          CoordTransform_const_sptr((*it)->clone()));
    else
      m_transforms_FromOriginal.push_back(CoordTransform_const_sptr());
  }

  for (it = other.m_transforms_ToOriginal.begin();
       it != other.m_transforms_ToOriginal.end(); ++it) {
    if (*it)
      m_transforms_ToOriginal.push_back(
          CoordTransform_const_sptr((*it)->clone()));
    else
      m_transforms_ToOriginal.push_back(CoordTransform_const_sptr());
  }

  // Copy the references to the original workspaces
  // This will also set up the delete observer to listen to those workspaces
  // being deleted.
  for (size_t i = 0; i < other.m_originalWorkspaces.size(); i++)
    this->setOriginalWorkspace(other.m_originalWorkspaces[i], i);
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
MDGeometry::~MDGeometry() {

  if (m_observingDelete) {
    // Stop watching once object is deleted
    API::AnalysisDataService::Instance().notificationCenter.removeObserver(
        m_delete_observer);
  }
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
    throw std::runtime_error("Workspace does not have a Z dimension.");
  return this->getDimension(2);
}

/// Get the t-dimension mapping.
boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MDGeometry::getTDimension() const {
  if (this->getNumDims() < 4)
    throw std::runtime_error("Workspace does not have a T dimension.");
  return this->getDimension(3);
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
    if (min < max)
      dim->setRange(dim->getNBins(), min, max);
    else
      dim->setRange(dim->getNBins(), max, min);
  }
  // Clear the original workspace
  setOriginalWorkspace(boost::shared_ptr<Workspace>());
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
 * @param notice :: notification of workspace deletion
 */
void MDGeometry::deleteNotificationReceived(
    Mantid::API::WorkspacePreDeleteNotification_ptr notice) {
  for (auto &original : m_originalWorkspaces) {
    if (original) {
      // Compare the pointer being deleted to the one stored as the original.
      Workspace_sptr deleted = notice->object();
      if (original == deleted) {
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
Mantid::API::CoordTransform const *
MDGeometry::getTransformFromOriginal(size_t index) const {
  if (index >= m_transforms_FromOriginal.size())
    throw std::runtime_error(
        "MDGeometry::getTransformFromOriginal(): invalid index.");
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
void MDGeometry::setTransformFromOriginal(
    Mantid::API::CoordTransform *transform, size_t index) {
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
Mantid::API::CoordTransform const *
MDGeometry::getTransformToOriginal(size_t index) const {
  if (index >= m_transforms_ToOriginal.size())
    throw std::runtime_error(
        "MDGeometry::getTransformFromOriginal(): invalid index.");
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
void MDGeometry::setTransformToOriginal(Mantid::API::CoordTransform *transform,
                                        size_t index) {
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

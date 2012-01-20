#include "MantidAPI/MDGeometry.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDGeometry::MDGeometry() :
   m_originalWorkspace(),
   m_transformFromOriginal(NULL), m_transformToOriginal(NULL),
   m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
   m_observingDelete(false)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Copy Constructor
   */
  MDGeometry::MDGeometry(const MDGeometry & other) :
   m_originalWorkspace(other.m_originalWorkspace),
   m_transformFromOriginal(NULL), m_transformToOriginal(NULL),
   m_delete_observer(*this, &MDGeometry::deleteNotificationReceived),
   m_observingDelete(false)
  {
    //TODO: How to copy the coordinate transformations?
    // Perform a deep copy of the dimensions
    std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions;
    for (size_t d=0; d < other.getNumDims(); d++)
    {
      // Copy the dimension
      MDHistoDimension_sptr dim(new MDHistoDimension( other.getDimension(d).get() ) );
      dimensions.push_back(dim);
    }
    this->initGeometry(dimensions);
  }

    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDGeometry::~MDGeometry()
  {
    if (m_transformFromOriginal)
      delete m_transformFromOriginal;
    if (m_transformToOriginal)
      delete m_transformToOriginal;
    if (m_observingDelete)
    {
      // Stop watching once object is deleted
      API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
    }
  }

  
  //----------------------------------------------------------------------------------------------
  /** Initialize the geometry
   *
   * @param dimensions :: vector of IMDDimension objects, in the order of X, Y, Z, t, etc.
   */
  void MDGeometry::initGeometry(std::vector<Mantid::Geometry::IMDDimension_sptr> & dimensions)
  {
    if (dimensions.size() == 0)
      throw std::invalid_argument("MDGeometry::initGeometry() 0 valid dimensions were given!");

    // Copy the dimensions array
    m_dimensions = dimensions;
    // Make sure the basis vectors are big enough
    m_basisVectors.resize(m_dimensions.size(), Mantid::Kernel::VMD());
  }


  // --------------------------------------------------------------------------------------------
  /** @return the number of dimensions in this workspace */
  size_t MDGeometry::getNumDims() const
  {
    return m_dimensions.size();
  }

  // --------------------------------------------------------------------------------------------
  /** Get a dimension
   * @param index :: which dimension
   * @return the dimension at that index
   */
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimension(size_t index) const
  {
    if (index >= m_dimensions.size()) throw std::runtime_error("Workspace does not have a dimension at that index.");
    return m_dimensions[index];
  }

  // --------------------------------------------------------------------------------------------
  /** Get a dimension
   * @param id :: string ID of the dimension
   * @return the dimension with the specified id string.
   */
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimensionNamed(std::string id) const
  {
    for (size_t i=0; i < m_dimensions.size(); ++i)
      if (m_dimensions[i]->getDimensionId() == id)
        return m_dimensions[i];
    throw std::invalid_argument("Dimension tagged " + id + " was not found in the Workspace");
  }

  // --------------------------------------------------------------------------------------------
  /** Get non-collapsed dimensions
  @return vector of collapsed dimensions in the workspace geometry.
  */
  Mantid::Geometry::VecIMDDimension_const_sptr MDGeometry::getNonIntegratedDimensions() const
  {
    using namespace Mantid::Geometry;
    VecIMDDimension_const_sptr vecCollapsedDimensions;
    std::vector<Mantid::Geometry::IMDDimension_sptr>::const_iterator it = this->m_dimensions.begin();
    for(; it != this->m_dimensions.end(); ++it)
    {
      IMDDimension_sptr current = (*it);
      if(!current->getIsIntegrated())
      {
        vecCollapsedDimensions.push_back(current);
      }
    }
    return vecCollapsedDimensions;
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the index of the dimension that matches the name given
   *
   * @param name :: name of the m_dimensions
   * @return the index (size_t)
   * @throw runtime_error if it cannot be found.
   */
  size_t MDGeometry::getDimensionIndexByName(const std::string & name) const
  {
    for (size_t d=0; d<m_dimensions.size(); d++)
      if (m_dimensions[d]->getName() == name)
        return d;
    throw std::runtime_error("Dimension named '" + name + "' was not found in the IMDWorkspace.");
  }

  // --------------------------------------------------------------------------------------------
  /** Add a dimension
   * @param dim :: shared pointer to the dimension object   */
  void MDGeometry::addDimension(boost::shared_ptr<Mantid::Geometry::IMDDimension> dim)
  {
    m_dimensions.push_back(dim);
  }

  // --------------------------------------------------------------------------------------------
  /** Add a dimension
   * @param dim :: bare pointer to the dimension object   */
  void MDGeometry::addDimension(Mantid::Geometry::IMDDimension * dim)
  {
    m_dimensions.push_back( boost::shared_ptr<Mantid::Geometry::IMDDimension>(dim) );
  }


  // --------------------------------------------------------------------------------------------
  /// Get the x-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getXDimension() const
  {
    if (this->getNumDims() < 1) throw std::runtime_error("Workspace does not have any dimensions!");
    return this->getDimension(0);
  }

  /// Get the y-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getYDimension() const
  {
    if (this->getNumDims() < 2) throw std::runtime_error("Workspace does not have a Y dimension.");
    return this->getDimension(1);
  }

  /// Get the z-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getZDimension() const
  {
    if (this->getNumDims() < 3) throw std::runtime_error("Workspace does not have a X dimension.");
    return this->getDimension(2);
  }

  /// Get the t-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getTDimension() const
  {
    if (this->getNumDims() < 4) throw std::runtime_error("Workspace does not have a T dimension.");
    return this->getDimension(3);
  }




  // --------------------------------------------------------------------------------------------
  /** Get the basis vector (in the original workspace) for a dimension of this workspace.
   * @param index :: which dimension
   * @return a vector, in the dimensions of the original workspace
   */
  Mantid::Kernel::VMD & MDGeometry::getBasisVector(size_t index)
  {
    if (index >= m_basisVectors.size())
      throw std::invalid_argument("getBasisVector(): invalid index");
    return m_basisVectors[index];
  }

  /** Get the basis vector (in the original workspace) for a dimension of this workspace.
   * @param index :: which dimension
   * @return a vector, in the dimensions of the original workspace
   */
  const Mantid::Kernel::VMD & MDGeometry::getBasisVector(size_t index) const
  {
    if (index >= m_basisVectors.size())
      throw std::invalid_argument("getBasisVector(): invalid index");
    return m_basisVectors[index];
  }

  /** Set the basis vector (in the original workspace) for a dimension of this workspace.
   * @param index :: which dimension
   * @param vec :: a vector, in the dimensions of the original workspace
   */
  void MDGeometry::setBasisVector(size_t index, const Mantid::Kernel::VMD & vec)
  {
    if (index >= m_basisVectors.size())
      throw std::invalid_argument("getBasisVector(): invalid index");
    m_basisVectors[index] = vec;
  }


  //---------------------------------------------------------------------------------------------------
  /// @return true if the geometry is defined relative to another workspace.
  bool MDGeometry::hasOriginalWorkspace() const
  {
    return bool(m_originalWorkspace);
  }

  /// @return the original workspace to which the basis vectors relate
  boost::shared_ptr<Workspace> MDGeometry::getOriginalWorkspace() const
  {
    return m_originalWorkspace;
  }

  /// Set the original workspace to which the basis vectors relate
  void MDGeometry::setOriginalWorkspace(boost::shared_ptr<Workspace> ws)
  {
    m_originalWorkspace = ws;
    // Watch for workspace deletions
    if (!m_observingDelete)
    {
        API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
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
  void MDGeometry::transformDimensions(std::vector<double> & scaling, std::vector<double> & offset)
  {
    if (scaling.size() != m_dimensions.size())
      throw std::invalid_argument("MDGeometry::transformDimensions(): scaling.size() must be equal to number of dimensions.");
    if (offset.size() != m_dimensions.size())
      throw std::invalid_argument("MDGeometry::transformDimensions(): offset.size() must be equal to number of dimensions.");
    for (size_t d=0; d<m_dimensions.size(); d++)
    {
      IMDDimension_sptr dim = m_dimensions[d];
      double min = (dim->getMinimum() * scaling[d]) + offset[d];
      double max = (dim->getMaximum() * scaling[d]) + offset[d];
      dim->setRange( dim->getNBins(), min, max);
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
  void MDGeometry::deleteNotificationReceived(Mantid::API::WorkspacePreDeleteNotification_ptr notice)
  {
    if (bool(m_originalWorkspace))
    {
      // Compare the pointer being deleted to the one stored as the original.
      Workspace_sptr deleted = notice->object();
      if (this->m_originalWorkspace == deleted)
      {
        // Clear the reference
        m_originalWorkspace.reset();
      }
    }
  }

  //---------------------------------------------------------------------------------------------------
  /// @return Coordinate Transformation that goes from the original workspace to this workspace's coordinates.
  Mantid::API::CoordTransform * MDGeometry::getTransformFromOriginal() const
  {
    return m_transformFromOriginal;
  }

  /** Set Coordinate Transformation that goes from the original workspace to this workspace's coordinates.
   * @param transform :: CoordTransform pointer (this assumes pointer ownership) */
  void MDGeometry::setTransformFromOriginal(Mantid::API::CoordTransform * transform)
  {
    if (m_transformFromOriginal)
      delete m_transformFromOriginal;
    m_transformFromOriginal = transform;
  }

  //---------------------------------------------------------------------------------------------------
  /// @return Coordinate Transformation that goes from this workspace's coordinates to the original workspace coordinates.
  Mantid::API::CoordTransform * MDGeometry::getTransformToOriginal() const
  {
    return m_transformToOriginal;
  }

  /** Set Coordinate Transformation that goes from this workspace's coordinates to the original workspace coordinates.
   * @param transform :: CoordTransform pointer (this assumes pointer ownership) */
  void MDGeometry::setTransformToOriginal(Mantid::API::CoordTransform * transform)
  {
    if (m_transformToOriginal)
      delete m_transformToOriginal;
    m_transformToOriginal = transform;
  }

  //---------------------------------------------------------------------------------------------------
  /** @return a XML representation of the geometry of the workspace */
  std::string MDGeometry::getGeometryXML() const
  {
    using Mantid::Geometry::MDGeometryBuilderXML;
    using Mantid::Geometry::NoDimensionPolicy;
    MDGeometryBuilderXML<NoDimensionPolicy> xmlBuilder;
    // Add all dimensions.
    const size_t nDimensions = this->getNumDims();
    for(size_t i = 0; i < nDimensions; i++)
    {
      xmlBuilder.addOrdinaryDimension(this->getDimension(i));
    }
    // Add mapping dimensions
    if(nDimensions > 0)
    {
     xmlBuilder.addXDimension(this->getXDimension());
    }
    if(nDimensions > 1)
    {
     xmlBuilder.addYDimension(this->getYDimension());
    }
    if(nDimensions > 2)
    {
     xmlBuilder.addZDimension(this->getZDimension());
    }
    if(nDimensions > 3)
    {
     xmlBuilder.addTDimension(this->getTDimension());
    }
     // Create the xml.
    return xmlBuilder.create();
  }


} // namespace Mantid
} // namespace API


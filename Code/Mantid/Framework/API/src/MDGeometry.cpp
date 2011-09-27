#include "MantidAPI/MDGeometry.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::MDGeometryBuilderXML;

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDGeometry::MDGeometry()
  :m_transformFromOriginal(NULL), m_transformToOriginal(NULL)
  {
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
  }

  
  //----------------------------------------------------------------------------------------------
  /** Initialize the geometry
   *
   * @param dimensions :: vector of MDDimension objects, in the order of X, Y, Z, t, etc.
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
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimensionNum(size_t index) const
  {
    if (index >= m_dimensions.size()) throw std::runtime_error("Workspace does not have a dimension at that index.");
    return m_dimensions[index];
  }

  // --------------------------------------------------------------------------------------------
  /** Get a dimension
   * @param id :: string ID of the dimension
   * @return the dimension with the specified id string.
   */
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimension(std::string id) const
  {
    for (size_t i=0; i < m_dimensions.size(); ++i)
      if (m_dimensions[i]->getDimensionId() == id)
        return m_dimensions[i];
    throw std::invalid_argument("Dimension tagged " + id + " was not found in the Workspace");
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
    return this->getDimensionNum(0);
  }

  /// Get the y-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getYDimension() const
  {
    if (this->getNumDims() < 2) throw std::runtime_error("Workspace does not have a Y dimension.");
    return this->getDimensionNum(1);
  }

  /// Get the z-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getZDimension() const
  {
    if (this->getNumDims() < 3) throw std::runtime_error("Workspace does not have a X dimension.");
    return this->getDimensionNum(2);
  }

  /// Get the t-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getTDimension() const
  {
    if (this->getNumDims() < 4) throw std::runtime_error("Workspace does not have a T dimension.");
    return this->getDimensionNum(3);
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
    return (m_originalWorkspace);
  }

  /// @return the original workspace to which the basis vectors relate
  boost::shared_ptr<IMDWorkspace> MDGeometry::getOriginalWorkspace() const
  {
    return m_originalWorkspace;
  }

  /// Set the original workspace to which the basis vectors relate
  void MDGeometry::setOriginalWorkspace(boost::shared_ptr<IMDWorkspace> ws)
  {
    m_originalWorkspace = ws;
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
    using Mantid::Geometry::StrictDimensionPolicy;
    MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
    // Add all dimensions.
    const size_t nDimensions = this->getNumDims();
    for(size_t i = 0; i < nDimensions; i++)
    {
      xmlBuilder.addOrdinaryDimension(this->getDimensionNum(i));
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


#include "MantidAPI/MDGeometry.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDGeometry::MDGeometry()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDGeometry::~MDGeometry()
  {
  }
  
  // --------------------------------------------------------------------------------------------
  /// Get the x-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getXDimension() const
  {
    if (m_dimensions.size() < 1) throw std::runtime_error("Workspace does not have any dimensions!");
    return m_dimensions[0];
  }

  /// Get the y-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getYDimension() const
  {
    if (m_dimensions.size() < 2) throw std::runtime_error("Workspace does not have a Y dimension.");
    return m_dimensions[1];
  }

  /// Get the z-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getZDimension() const
  {
    if (m_dimensions.size() < 3) throw std::runtime_error("Workspace does not have a X dimension.");
    return m_dimensions[2];
  }

  /// Get the t-dimension mapping.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getTDimension() const
  {
    if (m_dimensions.size() < 4) throw std::runtime_error("Workspace does not have a T dimension.");
    return m_dimensions[3];
  }

  // --------------------------------------------------------------------------------------------
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimensionNum(size_t index) const
  {
    if (index >= m_dimensions.size()) throw std::runtime_error("Workspace does not have a dimension at that index.");
    return m_dimensions[index];
  }

  // --------------------------------------------------------------------------------------------
  /// Get the dimension with the specified id.
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDGeometry::getDimension(std::string id) const
  {
    for (size_t i=0; i < m_dimensions.size(); ++i)
      if (m_dimensions[i]->getDimensionId() == id)
        return m_dimensions[i];
    throw std::invalid_argument("Dimension tagged " + id + " was not found in the Workspace");
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



} // namespace Mantid
} // namespace API


#include "MantidVatesAPI/IMDWorkspaceProxy.h"

namespace Mantid
{
namespace VATES
{
//-----------------------------------------------------------------------------------------------
/** New. Constructional method.
 * @param image. Image shared pointer.
 * @param xDim. X dimension Dimension_const_sptr.
 * @param yDim. Y dimension Dimension_const_sptr.
 * @param zDim. Z dimension Dimension_const_sptr.
 * @param tDim. t dimension Dimension_const_sptr.
 */

Mantid::API::IMDWorkspace_sptr IMDWorkspaceProxy::New(Mantid::API::IMDWorkspace_sptr workspace,
    Dimension_const_sptr xDim, Dimension_const_sptr yDim, Dimension_const_sptr zDim, Dimension_const_sptr tDim)
{
  IMDWorkspaceProxy* workspaceProxy = new IMDWorkspaceProxy(workspace, xDim, yDim, zDim, tDim);
  workspaceProxy->initalize();
  return Mantid::API::IMDWorkspace_sptr(workspaceProxy);
}

//-----------------------------------------------------------------------------------------------
/** find
 * @param key: key to find.
 */
IMDWorkspaceProxy::MemFuncGetter IMDWorkspaceProxy::find(const std::string& key) const
{
  std::map<std::string, MemFuncGetter>::const_iterator found = m_fmap.find(key);

  if (found != m_fmap.end())
  {
    MemFuncGetter memFunc = found->second;
    return memFunc;
  }
  else
  {
    throw std::runtime_error(std::string("Could not find in map: " + key));
  }
}

const std::string IMDWorkspaceProxy::id() const
{
  return m_workspace->id();
}

size_t IMDWorkspaceProxy::getMemorySize() const
 {
   return m_workspace->getMemorySize();
 }

uint64_t IMDWorkspaceProxy::getNPoints() const
{
  return m_workspace->getNPoints();
}

size_t IMDWorkspaceProxy::getNumDims() const
{
  return m_workspace->getNumDims();
}

boost::shared_ptr<const Mantid::Geometry::IMDDimension> IMDWorkspaceProxy::getDimension(std::string id) const
{
  return m_workspace->getDimension(id);
}

const std::vector<std::string> IMDWorkspaceProxy::getDimensionIDs() const
{
  return m_workspace->getDimensionIDs();
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getPoint(size_t index) const
{
  return m_workspace->getPoint(index);
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getCell(size_t dim1Increment) const
{
  return m_workspace->getCell(dim1Increment);
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getCell(size_t dim1Increment,
    size_t dim2Increment) const
{
  return m_workspace->getCell(dim1Increment, dim2Increment);
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getCell(size_t dim1Increment,
    size_t dim2Increment, size_t dim3Increment) const
{
  return m_workspace->getCell(dim1Increment, dim2Increment, dim3Increment);
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getCell(size_t dim1Increment,
    size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const
{
  return m_workspace->getCell(dim1Increment, dim2Increment, dim3Increment, dim4Increment);
}

const Mantid::Geometry::SignalAggregate& IMDWorkspaceProxy::getCell(...) const
{
  throw std::runtime_error("IMDWorkspaceProxy::getCell(...) Not Implemented");
}

std::string IMDWorkspaceProxy::getWSLocation() const
{
  throw std::runtime_error("IMDWorkspaceProxy::getWSLocation(...) Not Implemented");
}

std::string IMDWorkspaceProxy::getGeometryXML() const
{
  throw std::runtime_error("IMDWorkspaceProxy::getGeometryXML(...) Not Implemented");
}

//-----------------------------------------------------------------------------------------------
/** Initalization method. Creates a map of member functions to dimension ids.
 */

void IMDWorkspaceProxy::initalize()
{
  m_function = getMappedSignalAt();

  //Create a mapping of MDGeometry member functions to dimension ids. Keyed by the id.
  m_fmap[m_workspace->getXDimension()->getDimensionId()] = &IMDWorkspace::getXDimension;
  m_fmap[m_workspace->getYDimension()->getDimensionId()] = &IMDWorkspace::getYDimension;
  m_fmap[m_workspace->getZDimension()->getDimensionId()] = &IMDWorkspace::getZDimension;
  m_fmap[m_workspace->getTDimension()->getDimensionId()] = &IMDWorkspace::getTDimension;
}

//-----------------------------------------------------------------------------------------------
/** Constructor
 * @param image. Image shared pointer.
 * @param xDim. X dimension Dimension_const_sptr.
 * @param yDim. Y dimension Dimension_const_sptr.
 * @param zDim. Z dimension Dimension_const_sptr.
 * @param tDim. t dimension Dimension_const_sptr.
 */

IMDWorkspaceProxy::IMDWorkspaceProxy(Mantid::API::IMDWorkspace_sptr workspace, Dimension_const_sptr xDim,
    Dimension_const_sptr yDim, Dimension_const_sptr zDim, Dimension_const_sptr tDim

) :
  m_workspace(workspace), m_xDimension(xDim), m_yDimension(yDim), m_zDimension(zDim), m_tDimension(tDim)
{
}

IMDWorkspaceProxy::~IMDWorkspaceProxy()
{
}

Mantid::Geometry::IMDDimension_const_sptr IMDWorkspaceProxy::getXDimension(void) const
{
  //Find the effective xDimension
  MemFuncGetter mFunc = find(m_xDimension->getDimensionId());
  return (m_workspace.get()->*mFunc)();
}

Mantid::Geometry::IMDDimension_const_sptr IMDWorkspaceProxy::getYDimension(void) const
{
  //Find the effective yDimension
  MemFuncGetter mFunc = find(m_yDimension->getDimensionId());
  return (m_workspace.get()->*mFunc)();
}

Mantid::Geometry::IMDDimension_const_sptr IMDWorkspaceProxy::getZDimension(void) const
{
  //Find the effective zDimension
  MemFuncGetter mFunc = find(m_zDimension->getDimensionId());
  return (m_workspace.get()->*mFunc)();
}

Mantid::Geometry::IMDDimension_const_sptr IMDWorkspaceProxy::getTDimension(void) const
{
  //Find the effective tDimension
  MemFuncGetter mFunc = find(m_tDimension->getDimensionId());
  return (m_workspace.get()->*mFunc)();
}


double IMDWorkspaceProxy::getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const
{
  return m_function(index1, index2, index3, index4);
}

//-----------------------------------------------------------------------------------------------
/** Creates a remapping for ::getPoint member function of MDImage.
 * @returns A boost::bind as a boost::function in which one of the 4! possible parameter combinations is remapped correctly.
 */
boost::function<double(size_t, size_t, size_t, size_t)> IMDWorkspaceProxy::getMappedSignalAt()
{
  using namespace Mantid::API;
  //This switch is used to determine how to remap the arguments to the getPoint member function of MDImage.
  DimensionComparitor comparitor(m_workspace);

  //Handle binding correctly any one of 4! arrangements. TODO There may be a better way of meta-programming these 4! options.
  if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //xyzt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _2, _3, _4); //Default.
  }
  else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //xzyt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _3, _2, _4);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //yxzt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _1, _3, _4);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //yzxt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _3, _1, _4);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //zxyt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _1, _2, _4);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //zyxt
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _2, _1, _4);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //txyz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _1, _2, _3);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //txzy
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _1, _3, _2);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //tyxz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _2, _1, _3);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //tyzx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _2, _3, _1);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //tzxy
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _3, _1, _2);
  }
  else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //tzyx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _4, _3, _2, _1);
  }
  else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //xtyz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _4, _2, _3);
  }
  else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //xtzy
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _4, _3, _2);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //ytxz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _4, _1, _3);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //ytzx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _4, _3, _1);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //ztxy
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _4, _1, _2);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_tDimension)
      && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //ztyx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _4, _2, _1);
  }
  else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //xytz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _2, _4, _3);
  }
  else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //xzty
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _1, _3, _4, _2);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //yxtz
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _1, _4, _3);
  }
  else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_zDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //yztx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _2, _3, _4, _1);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_xDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //zxty
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _1, _4, _2);
  }
  else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_yDimension)
      && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //zytx
  {
    return boost::bind(&IMDWorkspace::getSignalAt, m_workspace, _3, _2, _4, _1);
  }
  else
  {
    throw std::runtime_error("Cannot generate a binding for ::getPoint");
  }
}

}
}

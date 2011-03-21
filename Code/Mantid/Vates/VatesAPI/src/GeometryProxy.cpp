#include "MantidVatesAPI/GeometryProxy.h"

#include "boost/regex.hpp"
#include "boost/function.hpp"
#include "boost/bind.hpp"

namespace Mantid
{
namespace VATES
{

bool isQxDimension(Mantid::Geometry::IMDDimension_sptr dimension)
{
  boost::regex xDimensionMatch("(q1)|(qx)");
  return boost::regex_match(dimension->getDimensionId(), xDimensionMatch);
}

bool isQyDimension(Mantid::Geometry::IMDDimension_sptr dimension)
{
  boost::regex yDimensionMatch("(q2)|(qy)");
  return boost::regex_match(dimension->getDimensionId(), yDimensionMatch);
}

bool isQzDimension(Mantid::Geometry::IMDDimension_sptr dimension)
{
  boost::regex zDimensionMatch("(q3)|(qz)");
  return boost::regex_match(dimension->getDimensionId(), zDimensionMatch);
}


GeometryProxy::MemFuncGetter GeometryProxy::find(std::string key) const
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

  GeometryProxy* GeometryProxy::New(Mantid::Geometry::MDGeometry* geometry, Dimension_sptr  xDim, Dimension_sptr  yDim,
      Dimension_sptr  zDim, Dimension_sptr  tDim)
  {
    GeometryProxy* geometryProxy = new GeometryProxy(geometry, xDim, yDim, zDim, tDim);
    geometryProxy->initalize();
    return geometryProxy;
  }

  void GeometryProxy::initalize()
  {
    //Create a mapping of MDGeometry member functions to dimension ids. Keyed by the id.
    m_fmap[m_geometry->getXDimension()->getDimensionId()] = &Geometry::getXDimension;
    m_fmap[m_geometry->getYDimension()->getDimensionId()] = &Geometry::getYDimension;
    m_fmap[m_geometry->getZDimension()->getDimensionId()] = &Geometry::getZDimension;
    m_fmap[m_geometry->getTDimension()->getDimensionId()] = &Geometry::getTDimension;
  }

  GeometryProxy::GeometryProxy(Mantid::Geometry::MDGeometry* geometry, Dimension_sptr  xDim, Dimension_sptr  yDim,
      Dimension_sptr  zDim, Dimension_sptr  tDim

  ) :
    m_geometry(geometry), m_xDimension(xDim), m_yDimension(yDim), m_zDimension(zDim), m_tDimension(tDim)
  {
  }

  GeometryProxy::~GeometryProxy()
  {
  }

  Dimension_sptr GeometryProxy::getXDimension(void) const
  {
    //Find the effective xDimension
    MemFuncGetter mFunc = find(m_xDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }
  Dimension_sptr GeometryProxy::getYDimension(void) const
  {
    //Find the effective yDimension
    MemFuncGetter mFunc = find(m_yDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }
  Dimension_sptr GeometryProxy::getZDimension(void) const
  {
    //Find the effective zDimension
    MemFuncGetter mFunc = find(m_zDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }
  Dimension_sptr GeometryProxy::getTDimension(void) const
  {
    //Find the effective tDimension
    MemFuncGetter mFunc = find(m_tDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }

  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> GeometryProxy::getMappedPointFunction(
      Mantid::MDDataObjects::MDImage_sptr image)
  {
    //This switch is used to determine how to remap the arguments to the getPoint member function of MDImage.
    using Mantid::MDDataObjects::MDImage;
    if (isQxDimension(m_xDimension) && isQyDimension(m_yDimension) && isQzDimension(m_zDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _1, _2, _3, _4);
    }
    else if (isQxDimension(m_xDimension) && isQyDimension(m_zDimension) && isQzDimension(m_yDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _1, _3, _2, _4);
    }
    else if (isQxDimension(m_yDimension) && isQyDimension(m_xDimension) && isQzDimension(m_zDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _2, _1, _3, _4);
    }
    else if (isQxDimension(m_yDimension) && isQyDimension(m_zDimension) && isQzDimension(m_xDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _2, _3, _1, _4);
    }
    else if (isQxDimension(m_zDimension) && isQyDimension(m_xDimension) && isQzDimension(m_yDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _3, _1, _2, _4);
    }
    else if (isQxDimension(m_zDimension) && isQyDimension(m_yDimension) && isQzDimension(m_xDimension))
    {
      return boost::bind(&MDImage::getPoint, image, _3, _2, _1, _4);
    }
    else
    {
      throw std::runtime_error("Cannot generate a binding for ::getPoint");
    }
  }

}
}

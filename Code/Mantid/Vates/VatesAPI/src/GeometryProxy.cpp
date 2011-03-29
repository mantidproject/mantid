#include "MantidVatesAPI/GeometryProxy.h"
#include "MantidVatesAPI/DimensionComparitor.h"

#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace Mantid
{
namespace VATES
{


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
    DimensionComparitor comparitor(image);

    //Handle binding correctly any one of 4! arrangements. TODO There may be a better way of meta-programming these 4! options.
    if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //xyzt
    {
      return boost::bind(&MDImage::getPoint, image, _1, _2, _3, _4); //Default.
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //xzyt
    {
      return boost::bind(&MDImage::getPoint, image, _1, _3, _2, _4);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //yxzt
    {
      return boost::bind(&MDImage::getPoint, image, _2, _1, _3, _4);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //yzxt
    {
      return boost::bind(&MDImage::getPoint, image, _2, _3, _1, _4);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //zxyt
    {
      return boost::bind(&MDImage::getPoint, image, _3, _1, _2, _4);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //zyxt
    {
      return boost::bind(&MDImage::getPoint, image, _3, _2, _1, _4);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //txyz
    {
      return boost::bind(&MDImage::getPoint, image, _4, _1, _2, _3);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //txzy
    {
      return boost::bind(&MDImage::getPoint, image, _4, _1, _3, _2);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //tyxz
    {
      return boost::bind(&MDImage::getPoint, image, _4, _2, _1, _3);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //tyzx
    {
      return boost::bind(&MDImage::getPoint, image, _4, _2, _3, _1);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //tzxy
    {
      return boost::bind(&MDImage::getPoint, image, _4, _3, _1, _2);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //tzyx
    {
      return boost::bind(&MDImage::getPoint, image, _4, _3, _2, _1);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //xtyz
    {
      return boost::bind(&MDImage::getPoint, image, _1, _4, _2, _3);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //xtzy
    {
      return boost::bind(&MDImage::getPoint, image, _1, _4, _3, _2);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //ytxz
    {
      return boost::bind(&MDImage::getPoint, image, _2, _4, _1, _3);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //ytzx
    {
      return boost::bind(&MDImage::getPoint, image, _2, _4, _3, _1);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //ztxy
    {
      return boost::bind(&MDImage::getPoint, image, _3, _4, _1, _2);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isXDimension(m_tDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //ztyx
    {
      return boost::bind(&MDImage::getPoint, image, _3, _4, _2, _1);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isXDimension(m_yDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //xytz
    {
      return boost::bind(&MDImage::getPoint, image, _1, _2, _4, _3);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isXDimension(m_zDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //xzty
    {
      return boost::bind(&MDImage::getPoint, image, _1, _3, _4, _2);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isXDimension(m_xDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //yxtz
    {
      return boost::bind(&MDImage::getPoint, image, _2, _1, _4, _3);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isXDimension(m_zDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //yztx
    {
      return boost::bind(&MDImage::getPoint, image, _2, _3, _4, _1);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isXDimension(m_xDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //zxty
    {
      return boost::bind(&MDImage::getPoint, image, _3, _1, _4, _2);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isXDimension(m_yDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //zytx
    {
      return boost::bind(&MDImage::getPoint, image, _3, _2, _4, _1);
    }
    else
    {
      throw std::runtime_error("Cannot generate a binding for ::getPoint");
    }
  }

}
}

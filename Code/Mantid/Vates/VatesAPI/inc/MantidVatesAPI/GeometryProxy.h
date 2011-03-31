#ifndef MANTID_VATES_GEOMETRYPROXY_H_
#define MANTID_VATES_GEOMETRYPROXY_H_

#include "MantidVatesAPI/Common.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidKernel/System.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include <boost/scoped_ptr.hpp>
#include <map>

#include "MantidVatesAPI/DimensionComparitor.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"

namespace Mantid
{
namespace VATES
{

/** Proxy for geometry. Allows the dimension related data to be fetched from an underlying geometry object in a runtime flexible fashion.
 * Ultimately reduces the need for rebinning operations where dimensions are simply remapped.
 *

@author Owen Arnold, Tessella plc
@date 21/03/2011

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

template<typename Image>
class DLLExport GeometryProxy
{

public:
  /// type definitions for member function returning Dimension_sptr
  //typedef Dimension_sptr (Mantid::Geometry::MDGeometry::*MemFuncGetter)() const;

  typedef typename Image::GeometryType::Dimension_sptr_type (Image::GeometryType::*MemFuncGetter)() const;

  typedef typename Image::GeometryType Geometry;

  /// Constructional method.
  static GeometryProxy* New(boost::shared_ptr<Image> image, Dimension_sptr  xDim, Dimension_sptr  yDim,
  Dimension_sptr  zDim, Dimension_sptr  tDim);

  /// Destructor.
  ~GeometryProxy();

  /// Get X Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getXDimension(void) const;
  /// Get Y Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getYDimension(void) const;
  /// Get Z Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getZDimension(void) const;
  /// Get t Dimension
  Dimension_sptr getTDimension(void) const;

  ///Returns a function wrapper bound to the the MDImage getPoint method, which is rebound depending upon the dimension arrangement.
  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> getMappedPointFunction();

private:

  /// Constructor.
  GeometryProxy(boost::shared_ptr<Image> image, Dimension_sptr  xDim, Dimension_sptr  yDim,
        Dimension_sptr  zDim, Dimension_sptr  tDim);

  /// Separate initalizer as may throw.
  void initalize();

  /// Wrapped geometry for this proxy.
  typename Image::GeometryType* m_geometry;
  /// Actual x dimension
  Dimension_sptr m_xDimension;
  /// Actual y dimension
  Dimension_sptr m_yDimension;
  /// Actual z dimension
  Dimension_sptr m_zDimension;
  /// Actual t dimension
  Dimension_sptr m_tDimension;
  /// map of id to member function.
  std::map<std::string, MemFuncGetter > m_fmap;
  /// image shared ptr.
  boost::shared_ptr<Image> m_image;

  GeometryProxy(const GeometryProxy&); // Not implemented.
  void operator=(const GeometryProxy&); // Not implemented.

public:

//-----------------------------------------------------------------------------------------------
/** find
* @param key: key to find.
*/
  MemFuncGetter find(const std::string& key) const
  {
    typename std::map<std::string, MemFuncGetter>::const_iterator found;
    found = m_fmap.find(key);

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

};

//-----------------------------------------------------------------------------------------------
/** New. Constructional method.
* @param image. Image shared pointer.
* @param xDim. X dimension Dimension_sptr.
* @param yDim. Y dimension Dimension_sptr.
* @param zDim. Z dimension Dimension_sptr.
* @param tDim. t dimension Dimension_sptr.
*/
template<typename Image>
  GeometryProxy<Image>* GeometryProxy<Image>::New(boost::shared_ptr<Image> image, Dimension_sptr  xDim, Dimension_sptr  yDim,
      Dimension_sptr  zDim, Dimension_sptr  tDim)
  {
    GeometryProxy* geometryProxy = new GeometryProxy(image, xDim, yDim, zDim, tDim);
    geometryProxy->initalize();
    return geometryProxy;
  }

//-----------------------------------------------------------------------------------------------
/** Initalization method. Creates a map of member functions to dimension ids.
*/
template<typename Image>
  void GeometryProxy<Image>::initalize()
  {
    //Create a mapping of MDGeometry member functions to dimension ids. Keyed by the id.
    m_fmap[m_xDimension->getDimensionId()] = &Geometry::getXDimension;
    m_fmap[m_yDimension->getDimensionId()] = &Geometry::getYDimension;
    m_fmap[m_zDimension->getDimensionId()] = &Geometry::getZDimension;
    m_fmap[m_tDimension->getDimensionId()] = &Geometry::getTDimension;
  }

//-----------------------------------------------------------------------------------------------
/** Constructor
* @param image. Image shared pointer.
* @param xDim. X dimension Dimension_sptr.
* @param yDim. Y dimension Dimension_sptr.
* @param zDim. Z dimension Dimension_sptr.
* @param tDim. t dimension Dimension_sptr.
*/
template<typename Image>
  GeometryProxy<Image>::GeometryProxy(boost::shared_ptr<Image> image, Dimension_sptr  xDim, Dimension_sptr  yDim,
      Dimension_sptr  zDim, Dimension_sptr  tDim

  ) : m_image(image), m_geometry(image->getGeometry()), m_xDimension(xDim), m_yDimension(yDim), m_zDimension(zDim), m_tDimension(tDim)
  {
  }

/// Destructor.
template<typename Image>
  GeometryProxy<Image>::~GeometryProxy()
  {
  }

/// Getter for the x dimension in its remapped form.
template<typename Image>
  Dimension_sptr GeometryProxy<Image>::getXDimension(void) const
  {
    //Find the effective xDimension
    MemFuncGetter mFunc = find(m_xDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }

/// Getter for the y dimension in its remapped form.
template<typename Image>
  Dimension_sptr GeometryProxy<Image>::getYDimension(void) const
  {
    //Find the effective yDimension
    MemFuncGetter mFunc = find(m_yDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }

/// Getter for the z dimension in its remapped form.
template<typename Image>
  Dimension_sptr GeometryProxy<Image>::getZDimension(void) const
  {
    //Find the effective zDimension
    MemFuncGetter mFunc = find(m_zDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }

/// Getter for the t dimension in its remapped form.
template<typename Image>
  Dimension_sptr GeometryProxy<Image>::getTDimension(void) const
  {
    //Find the effective tDimension
    MemFuncGetter mFunc = find(m_tDimension->getDimensionId());
    return (m_geometry->*mFunc)();
  }

//-----------------------------------------------------------------------------------------------
/** Creates a remapping for ::getPoint member function of MDImage.
* @returns A boost::bind as a boost::function in which one of the 4! possible parameter combinations is remapped correctly.
*/
template<typename Image>
  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> GeometryProxy<Image>::getMappedPointFunction() 
  {
    //This switch is used to determine how to remap the arguments to the getPoint member function of MDImage.
    DimensionComparitor<Image> comparitor(m_image);

    //Handle binding correctly any one of 4! arrangements. TODO There may be a better way of meta-programming these 4! options.
    if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //xyzt
    {
      return boost::bind(&Image::getPoint, m_image, _1, _2, _3, _4); //Default.
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //xzyt
    {
      return boost::bind(&Image::getPoint, m_image, _1, _3, _2, _4);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_tDimension)) //yxzt
    {
      return boost::bind(&Image::getPoint, m_image, _2, _1, _3, _4);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //yzxt
    {
      return boost::bind(&Image::getPoint, m_image, _2, _3, _1, _4);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_tDimension)) //zxyt
    {
      return boost::bind(&Image::getPoint, m_image, _3, _1, _2, _4);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_tDimension)) //zyxt
    {
      return boost::bind(&Image::getPoint, m_image, _3, _2, _1, _4);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //txyz
    {
      return boost::bind(&Image::getPoint, m_image, _4, _1, _2, _3);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //txzy
    {
      return boost::bind(&Image::getPoint, m_image, _4, _1, _3, _2);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //tyxz
    {
      return boost::bind(&Image::getPoint, m_image, _4, _2, _1, _3);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //tyzx
    {
      return boost::bind(&Image::getPoint, m_image, _4, _2, _3, _1);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //tzxy
    {
      return boost::bind(&Image::getPoint, m_image, _4, _3, _1, _2);
    }
    else if (comparitor.isXDimension(m_tDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //tzyx
    {
      return boost::bind(&Image::getPoint, m_image, _4, _3, _2, _1);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_zDimension)) //xtyz
    {
      return boost::bind(&Image::getPoint, m_image, _1, _4, _2, _3);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_yDimension)) //xtzy
    {
      return boost::bind(&Image::getPoint, m_image, _1, _4, _3, _2);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_zDimension)) //ytxz
    {
      return boost::bind(&Image::getPoint, m_image, _2, _4, _1, _3);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_zDimension) && comparitor.istDimension(m_xDimension)) //ytzx
    {
      return boost::bind(&Image::getPoint, m_image, _2, _4, _3, _1);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_xDimension) && comparitor.istDimension(m_yDimension)) //ztxy
    {
      return boost::bind(&Image::getPoint, m_image, _3, _4, _1, _2);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_tDimension) && comparitor.isZDimension(m_yDimension) && comparitor.istDimension(m_xDimension)) //ztyx
    {
      return boost::bind(&Image::getPoint, m_image, _3, _4, _2, _1);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //xytz
    {
      return boost::bind(&Image::getPoint, m_image, _1, _2, _4, _3);
    }
    else if (comparitor.isXDimension(m_xDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //xzty
    {
      return boost::bind(&Image::getPoint, m_image, _1, _3, _4, _2);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_zDimension)) //yxtz
    {
      return boost::bind(&Image::getPoint, m_image, _2, _1, _4, _3);
    }
    else if (comparitor.isXDimension(m_yDimension) && comparitor.isYDimension(m_zDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //yztx
    {
      return boost::bind(&Image::getPoint, m_image, _2, _3, _4, _1);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_xDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_yDimension)) //zxty
    {
      return boost::bind(&Image::getPoint, m_image, _3, _1, _4, _2);
    }
    else if (comparitor.isXDimension(m_zDimension) && comparitor.isYDimension(m_yDimension) && comparitor.isZDimension(m_tDimension) && comparitor.istDimension(m_xDimension)) //zytx
    {
      return boost::bind(&Image::getPoint, m_image, _3, _2, _4, _1);
    }
    else
    {
      throw std::runtime_error("Cannot generate a binding for ::getPoint");
    }
  }
}
}

#endif

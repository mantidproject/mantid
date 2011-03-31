#ifndef MANTID_VATES_IMAGEPROCESSOR_H_
#define MANTID_VATES_IMAGEPROCESSOR_H_

#include "MantidKernel/System.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidVatesAPI/GeometryProxy.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
namespace VATES
{

/** Proxy for a md image. Uses a geometry proxy to re-wire calls to getPoint in a manner that allow runtime flexiblity in the argument order.

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
class DLLExport ImageProxy
{
private:

  /// Assisting mdgeometry proxy. Constructs and contains all remapping information.
  boost::scoped_ptr<GeometryProxy<Image> > m_geometryProxy;

  /// underlying MDImage. The subject of this proxy.
  Mantid::MDDataObjects::MDImage_sptr m_image;

  /// Cached member function provided by geometryProxy.
  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> m_Function;

  /// Constructor.
  ImageProxy(GeometryProxy<Image>* geometryProxy, boost::shared_ptr<Image> image);

  /// Setup method. may throw.
  void initalize();

  ImageProxy(const ImageProxy&); // Not implemented.
  void operator=(const ImageProxy&); // Not implemented.

public:

  /// Constructional method.
  static ImageProxy* New(GeometryProxy<Image>* geometryProxy, boost::shared_ptr<Image> image);

  /// Destructor
 ~ImageProxy();

  /// Embedded type information. Used for static polymorphism.
  typedef GeometryProxy<Image> GeometryType;

  /// Method from MDImage. Statically overriden here.
  GeometryProxy<Image> * const getGeometry();

  /// Method from MDImage. Statically overriden here.
  Mantid::MDDataObjects::MD_image_point getPoint(int i, int j, int k, int t) const;

  /// Get the real image.
  boost::shared_ptr<Image> getRealImage() const;

};

//-----------------------------------------------------------------------------------------------
/** Constructional method.
* @param geometryProxy :: GeometryProxy<Image>* that may wrap a geometry in order to provide rebining capabilities.
* @param image :: image sharped pointer, points to the image that this proxy wraps.
*/
template<typename Image>
ImageProxy<Image>* ImageProxy<Image>::New(GeometryProxy<Image>* geometryProxy, boost::shared_ptr<Image> image)
{
  ImageProxy<Image>* imageProcessor = new ImageProxy<Image>(geometryProxy, image);
  imageProcessor->initalize(); //This way initalize may throw rather than the constructor.
  return imageProcessor;
}

//-----------------------------------------------------------------------------------------------
/** Constructor
* @param geometryProxy :: GeometryProxy<Image>* that may wrap a geometry in order to provide rebining capabilities.
* @param image :: image sharped pointer, points to the image that this proxy wraps.
*/
template<typename Image>
ImageProxy<Image>::ImageProxy(GeometryProxy<Image>* geometryProxy, boost::shared_ptr<Image> image) :
m_geometryProxy(geometryProxy), m_image(image)
{
}

//-----------------------------------------------------------------------------------------------
/** Destructor.*/
template<typename Image>
ImageProxy<Image>::~ImageProxy()
{
}

//-----------------------------------------------------------------------------------------------
/** getGeometry, getter to support compile-time polymorphism. Gets underlying geometry type*/
template<typename Image>
GeometryProxy<Image> * const ImageProxy<Image>::getGeometry()
{
  return m_geometryProxy.get();
}

//-----------------------------------------------------------------------------------------------
/** getPoint
* @param i : value of increment along 1st dimension in the context of whatever rebinnings have been applied. effective x Dimension.
* @param j : value of increment along 2st dimension in the context of whatever rebinnings have been applied. effective y Dimension.
* @param k : value of increment along 3st dimension in the context of whatever rebinnings have been applied. effective z Dimension.
* @param t : value of increment along 4st dimension in the context of whatever rebinnings have been applied. effective t Dimension.
*/
template<typename Image>
Mantid::MDDataObjects::MD_image_point ImageProxy<Image>::getPoint(int i, int j, int k, int t) const
{

  //Re-route
  return m_Function(i, j, k, t);
}

//-----------------------------------------------------------------------------------------------
/** initalize()
* This method critically uses to geometry to determine the rebindings of the getpoint method and then stores the resulting boost::function as a member function.
*/
template<typename Image>
void ImageProxy<Image>::initalize()
{
  //Here's the trick. Get a mapped point function from the geometry proxy.
  m_Function = m_geometryProxy->getMappedPointFunction();
}


//-----------------------------------------------------------------------------------------------
/** getRealImage()
* This gets the underlying image.
*/
template<typename Image>
boost::shared_ptr<Image> ImageProxy<Image>::getRealImage() const
{
  return m_image;
}


}
}

#endif

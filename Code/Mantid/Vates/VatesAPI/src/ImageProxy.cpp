#include "MantidVatesAPI/ImageProxy.h"
#include <boost/function.hpp>

namespace Mantid
{
namespace VATES
{

  ImageProxy* ImageProxy::New(GeometryProxy* geometryProxy, Mantid::MDDataObjects::MDImage_sptr image)
    {
      ImageProxy* imageProcessor = new ImageProxy(geometryProxy, image);
      imageProcessor->initalize(); //This way initalize may throw rather than the constructor.
      return imageProcessor;
    }

  ImageProxy::ImageProxy(GeometryProxy* geometryProxy, Mantid::MDDataObjects::MDImage_sptr image) :
    m_geometryProxy(geometryProxy), m_image(image)
  {
  }

  ImageProxy::~ImageProxy()
  {
  }

  GeometryProxy * const ImageProxy::getGeometry()
  {
    return m_geometryProxy.get();
  }

  Mantid::MDDataObjects::MD_image_point ImageProxy::getPoint(int i, int j, int k, int t) const
  {

    //Re-route
    return m_Function(i, j, k, t);
  }

  void ImageProxy::initalize()
  {
    //Here's the trick. Get a mapped point function from the geometry proxy.
    m_Function = m_geometryProxy->getMappedPointFunction(m_image);
  }

}
}

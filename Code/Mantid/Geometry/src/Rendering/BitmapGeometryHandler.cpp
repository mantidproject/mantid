#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

#include <climits>
#ifdef _WIN32
#include "windows.h"
#endif
#include "GL/gl.h"
#include "GL/glu.h"

namespace Mantid
{
namespace Geometry
{


  BitmapGeometryHandler::BitmapGeometryHandler(RectangularDetector *comp)
  : GeometryHandler(dynamic_cast<IObjComponent*>(comp))
  {
    //Save the rectangular detector link for later.
    mRectDet = comp;
  }


  BitmapGeometryHandler::BitmapGeometryHandler() : GeometryHandler((Object *)NULL)
  {

  }


  /// Destructor
  BitmapGeometryHandler::~BitmapGeometryHandler()
  {

  }

  ///< Create an instance of concrete geometry handler for ObjComponent
  BitmapGeometryHandler* BitmapGeometryHandler::createInstance(IObjComponent *comp)
  {
    return new BitmapGeometryHandler(); //comp);
  }

  ///< Create an instance of concrete geometry handler for Object
  BitmapGeometryHandler* BitmapGeometryHandler::createInstance(boost::shared_ptr<Object> obj )
  {
    return new BitmapGeometryHandler(); //obj);
  }

  //----------------------------------------------------------------------------------------------
  /** Triangulate the Object - this function will not be used.
   *
   */
  void BitmapGeometryHandler::Triangulate()
  {
    std::cout << "BitmapGeometryHandler::Triangulate() called\n";
  }

  //----------------------------------------------------------------------------------------------
  ///< Render Object or ObjComponent
  void BitmapGeometryHandler::Render()
  {
    std::cout << "BitmapGeometryHandler::Render() called\n";
    V3D pos;

    //Wait for no error
    while(glGetError() != GL_NO_ERROR);

    glBindTexture (GL_TEXTURE_2D, 13);
    glBegin (GL_QUADS);

    glTexCoord2f (0.0, 0.0);
    pos = mRectDet->getRelativePosAtXY(0,0);
    glVertex3f( pos.X(), pos.Y(), pos.Z());

    glTexCoord2f (1.0, 0.0);
    pos = mRectDet->getRelativePosAtXY(mRectDet->xpixels()-1,0);
    glVertex3f( pos.X(), pos.Y(), pos.Z());

    glTexCoord2f (1.0, 1.0);
    pos = mRectDet->getRelativePosAtXY(mRectDet->xpixels()-1, mRectDet->ypixels()-1);
    glVertex3f( pos.X(), pos.Y(), pos.Z());

    glTexCoord2f (0.0, 1.0);
    pos = mRectDet->getRelativePosAtXY(0, mRectDet->ypixels()-1);
    glVertex3f( pos.X(), pos.Y(), pos.Z());


    glEnd ();

  }

  //----------------------------------------------------------------------------------------------
 ///< Prepare/Initialize Object/ObjComponent to be rendered
  void BitmapGeometryHandler::Initialize()
  {
    std::cout << "BitmapGeometryHandler::Initialize() called\n";
  }


}
}

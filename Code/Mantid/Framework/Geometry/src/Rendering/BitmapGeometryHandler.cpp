#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <climits>
#include <iostream>

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{
  using Kernel::V3D;

  /**
   * @return A shared_ptr to a new copy of this object
   */
  boost::shared_ptr<GeometryHandler> BitmapGeometryHandler::clone() const
  {
    return boost::make_shared<BitmapGeometryHandler>(*this);
  }

  /// Parameter constructor
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
    (void)comp;
    return new BitmapGeometryHandler();
  }

  ///< Create an instance of concrete geometry handler for Object
  BitmapGeometryHandler* BitmapGeometryHandler::createInstance(boost::shared_ptr<Object> obj )
  {
    (void)obj;
    return new BitmapGeometryHandler();
  }

  ///< Create an instance of concrete geometry handler for Object
  GeometryHandler* BitmapGeometryHandler::createInstance(Object* obj )
  {
    (void)obj;
    return new BitmapGeometryHandler();
  }

  //----------------------------------------------------------------------------------------------
  /** Triangulate the Object - this function will not be used.
   *
   */
  void BitmapGeometryHandler::Triangulate()
  {
    //std::cout << "BitmapGeometryHandler::Triangulate() called\n";
  }

  //----------------------------------------------------------------------------------------------
  ///< Render Object or ObjComponent
  void BitmapGeometryHandler::Render()
  {
    //std::cout << "BitmapGeometryHandler::Render() called\n";
    V3D pos;

    //Wait for no error
    while(glGetError() != GL_NO_ERROR);

    // Because texture colours are combined with the geometry colour
    // make sure the current colour is white
    glColor3f(1.0f,1.0f,1.0f);

    //Nearest-neighbor scaling
    GLint texParam = GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texParam);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texParam);

    glEnable (GL_TEXTURE_2D); // enable texture mapping

    int texx, texy;
    mRectDet->getTextureSize(texx, texy);
    double tex_frac_x = (1.0 * mRectDet->xpixels()) / (texx);
    double tex_frac_y = (1.0 * mRectDet->ypixels()) / (texy);

    //Point to the ID of the texture that was created before - in RectangularDetectorActor.
    //int texture_id = mRectDet->getTextureID();
    //glBindTexture (GL_TEXTURE_2D, texture_id);
    //if (glGetError()>0) std::cout << "OpenGL error in glBindTexture \n";

    glBegin (GL_QUADS);

    glTexCoord2f (0.0, 0.0);
    pos = mRectDet->getRelativePosAtXY(0,0);
    pos += V3D( mRectDet->xstep()*(-0.5), mRectDet->ystep()*(-0.5), 0.0); //Adjust to account for the size of a pixel
    glVertex3f( (GLfloat)pos.X(), (GLfloat)pos.Y(), (GLfloat)pos.Z());

    glTexCoord2f ((GLfloat)tex_frac_x, 0.0);
    pos = mRectDet->getRelativePosAtXY(mRectDet->xpixels()-1,0);
    pos += V3D( mRectDet->xstep()*(+0.5), mRectDet->ystep()*(-0.5), 0.0); //Adjust to account for the size of a pixel
    glVertex3f( (GLfloat)pos.X(), (GLfloat)pos.Y(), (GLfloat)pos.Z());

    glTexCoord2f ((GLfloat)tex_frac_x, (GLfloat)tex_frac_y);
    pos = mRectDet->getRelativePosAtXY(mRectDet->xpixels()-1, mRectDet->ypixels()-1);
    pos += V3D( mRectDet->xstep()*(+0.5), mRectDet->ystep()*(+0.5), 0.0); //Adjust to account for the size of a pixel
    glVertex3f( (GLfloat)pos.X(), (GLfloat)pos.Y(), (GLfloat)pos.Z());

    glTexCoord2f (0.0, (GLfloat)tex_frac_y);
    pos = mRectDet->getRelativePosAtXY(0, mRectDet->ypixels()-1);
    pos += V3D( mRectDet->xstep()*(-0.5), mRectDet->ystep()*(+0.5), 0.0); //Adjust to account for the size of a pixel
    glVertex3f( (GLfloat)pos.X(), (GLfloat)pos.Y(), (GLfloat)pos.Z());

    glEnd ();
    if (glGetError()>0) std::cout << "OpenGL error in BitmapGeometryHandler::Render \n";

    glDisable (GL_TEXTURE_2D); // stop texture mapping - not sure if this is necessary.

  }

  //----------------------------------------------------------------------------------------------
 ///< Prepare/Initialize Object/ObjComponent to be rendered
  void BitmapGeometryHandler::Initialize()
  {
    //std::cout << "BitmapGeometryHandler::Initialize() called\n";
  }


}
}

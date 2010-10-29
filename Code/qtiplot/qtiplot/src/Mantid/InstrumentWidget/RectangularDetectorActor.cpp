#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "RectangularDetectorActor.h"
#include "ObjComponentActor.h"
#include "RectangularDetectorActor.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;

static const bool VERBOSE = false;


/**
 * Constructor.
 *
 * @param withDisplayList :: true to create a display list
 */
RectangularDetectorActor::RectangularDetectorActor(boost::shared_ptr<Mantid::Geometry::IRectangularDetector> rectDet)
:
    ObjComponentActor(NULL, boost::dynamic_pointer_cast<IObjComponent>( rectDet ), false),
    mNumberOfDetectors(0),minBoundBox(DBL_MAX,DBL_MAX,DBL_MAX),maxBoundBox(-DBL_MAX,-DBL_MAX,-DBL_MAX)
{
  mDet = rectDet;

  if (mDet)
  {
    BoundingBox compBox;
    mDet->getBoundingBox(compBox);
    //std::cout << " RectangularDetectorActor : bounding box is " << compBox.minPoint() << " " << compBox.maxPoint() << "\n";
    this->AppendBoundingBox(compBox.minPoint(), compBox.maxPoint());
  }

}



//-------------------------------------------------------------------------------------------------
/**
 * Destructor which removes the actors created by this object
 */
RectangularDetectorActor::~RectangularDetectorActor()
{
}


//-------------------------------------------------------------------------------------------------
/**
 * This function is concrete implementation that renders the Child ObjComponents and Child CompAssembly's
 */
void RectangularDetectorActor::define()
{
  if (VERBOSE) std::cout << "RectangularDetectorActor::define() called for " << mDet->getName() << "\n";
  ObjComponentActor::define();
}


//-------------------------------------------------------------------------------------------------
/**
 * Concrete implementation of init method of GLObject. this method draws the children
 */
void RectangularDetectorActor::init()
{
  if (VERBOSE) std::cout << "RectangularDetectorActor::init() called for " << mDet->getName() << "\n";
}


//-------------------------------------------------------------------------------------------------
/**
 * This method redraws the CompAssembly children compassembly actors redraw. this method is used to redraw all the children
 */
void RectangularDetectorActor::redraw()
{
  if (VERBOSE) std::cout << "RectangularDetectorActor::redraw() called for " << mDet->getName() << "\n";
}

//-------------------------------------------------------------------------------------------------
/** Append the detector ID of this object to the list, if it is a detector.
 *
 * @param idList sequential list of detector IDs.
 */
void RectangularDetectorActor::appendObjCompID(std::vector<int>& idList)
{
  if (mDet)
  {
    //Append all the detector IDs of this rectangular detector
    if (VERBOSE) std::cout << "RectangularDetectorActor::appendObjCompID() called for " << mDet->getName() << "\n";
    for (int y=0; y < mDet->ypixels(); y++)
      for (int x=0; x < mDet->xpixels(); x++)
        idList.push_back( mDet->getAtXY(x,y)->getID() );

  }
}


//------------------------------------------------------------------------------------------------
/**
 * The colors are set using the iterator of the color list. The order of the detectors
 * in this color list was defined by the calls to appendObjCompID().
 *
 * @param list :: Color list iterator
 * @return the number of detectors
 */
int RectangularDetectorActor::setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator& list)
{
  int num = mDet->xpixels() * mDet->ypixels();

  //The texture size must be 2^n power.
  int text_x_size, text_y_size;
  mDet->getTextureSize(text_x_size, text_y_size);

  //------ Create the image data buffer -------
  char * image_data;
  image_data = new char[3*text_x_size*text_y_size];
  char * image_data_ptr = image_data;

  //Fill with 0 (black) everywhere
  std::fill(image_data, image_data + 3*text_x_size*text_y_size, 0);

  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      //Use black as the default color (for going off the edges)
      char r=0, g=0, b=0;
      //Get the current color
      float rf, gf, bf, af;
      (*list)->get(rf,gf,bf,af);
      //Convert to bytes
      r = (char) (255*rf);
      g = (char) (255*gf);
      b = (char) (255*bf);
      //Go to the next color
      list++;

//      //TEMP: way to show the colors
//      r=x;
//      g=y;
//      b=x;

      //Save the color data to the buffer
      *image_data_ptr = r;
      image_data_ptr++;
      *image_data_ptr = g;
      image_data_ptr++;
      *image_data_ptr = b;
      image_data_ptr++;

    }
    //Skip any padding in x. 3 bytes per pixel
    image_data_ptr += 3*(text_x_size-mDet->xpixels());
  }

  // Set up before uploading a texture
  int texture_id = mDet->getAtXY(0,0)->getID();
  glBindTexture(GL_TEXTURE_2D, texture_id);
  if (glGetError()>0) std::cout << "OpenGL error in glBindTexture \n";

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); //This one allows lighting effects
  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); //This one doesn't

  //Upload the texture to video card
  if (glGetError()>0) std::cout << "OpenGL error BEFORE glTexImage2D \n";
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, text_x_size, text_y_size, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
  if (glGetError()>0) std::cout << "OpenGL error in glTexImage2D \n";

  if (VERBOSE) std::cout << "RectangularDetectorActor::setInternalDetectorColors() called for " << mDet->getName() << " with " << num << " entries set\n";
  //Free memory - the bitmap has been uploaded to video memory.

  delete [] image_data;

  return num;


//  const boost::shared_ptr<Mantid::Geometry::IDetector>  detector =
//      boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>(this->getObjComponent());
//  if( detector != boost::shared_ptr<Mantid::Geometry::IDetector>() )
//  {
//    this->setColor((*list));
//    list++;
//    return 1;
//  }
//  else
//    return 0;

}



//-------------------------------------------------------------------------------------------------
/**
 * Return the bounding box, from the one calculated in the cache previously.
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void RectangularDetectorActor::getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)
{
  minBound=minBoundBox;
  maxBound=maxBoundBox;
}


/**
 * Append the bounding box CompAssembly bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void RectangularDetectorActor::AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound)
{
  if(minBoundBox[0]>minBound[0]) minBoundBox[0]=minBound[0];
  if(minBoundBox[1]>minBound[1]) minBoundBox[1]=minBound[1];
  if(minBoundBox[2]>minBound[2]) minBoundBox[2]=minBound[2];
  if(maxBoundBox[0]<maxBound[0]) maxBoundBox[0]=maxBound[0];
  if(maxBoundBox[1]<maxBound[1]) maxBoundBox[1]=maxBound[1];
  if(maxBoundBox[2]<maxBound[2]) maxBoundBox[2]=maxBound[2];
}


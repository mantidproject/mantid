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
RectangularDetectorActor::RectangularDetectorActor(boost::shared_ptr<Mantid::Geometry::RectangularDetector> rectDet)
:
    ICompAssemblyActor(false), mTextureID(0)
{
  mNumberOfDetectors = 0;
  mDet = rectDet;
  image_data = NULL;
  pick_data = NULL;

  if (mDet)
  {
    BoundingBox compBox;
    mDet->getBoundingBox(compBox);
    mNumberOfDetectors =  mDet->xpixels() * mDet->ypixels();
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

  glPushMatrix();
  // Translation first
  V3D pos = mDet->getPos();
  if (!(pos.nullVector()))
  {
    glTranslated(pos[0],pos[1],pos[2]);
  }
  //Rotation
  Quat rot = mDet->getRotation();
  if (!(rot.isNull()))
  {
    double deg,ax0,ax1,ax2;
    rot.getAngleAxis(deg,ax0,ax1,ax2);
    glRotated(deg,ax0,ax1,ax2);
  }
  //Scale
  V3D scaleFactor = mDet->getScaleFactor();
  if (!(scaleFactor==V3D(1,1,1)))
  {
    glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
  }

  // RectangularDetector will use.
  mDet->draw();

  glPopMatrix();
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
 * @param idList :: sequential list of detector IDs.
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
 * Set the starting color reference for CompAssembly
 * @param rgb :: input color id
 * @return  the number of color ids that are used.
 */
int RectangularDetectorActor::setStartingReferenceColor(int rgb)
{
  if (VERBOSE) std::cout << "RectangularDetectorActor::setStartingReferenceColor() called for " << mDet->getName() << " with rgb = " << rgb << "\n";
  mColorStartID=rgb;
  return this->getNumberOfDetectors();
}

//------------------------------------------------------------------------------------------------
/**
 * This method searches the child actors for the input rgb color and returns the detector id corresponding to the rgb color. if the
 * detector is not found then returns -1.
 * @param  rgb :: input color id
 * @return the detector id of the input rgb color.if detector id not found then returns -1
 */
int RectangularDetectorActor::findDetectorIDUsingColor(int rgb)
{
  if (VERBOSE) std::cout << "RectangularDetectorActor::findDetectorIDUsingColor() called for " << mDet->getName() << "\n";
  //The row and column of the detector can be found from the color
  int diff_rgb = rgb - mColorStartID;
  int y = diff_rgb / mDet->xpixels();
  int x = diff_rgb % mDet->xpixels();
  //And now we return that ID :)
  return mDet->getAtXY(x,y)->getID();
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
  int num = this->genTexture(image_data, list, false);
  this->uploadTexture(image_data);
  return num;
}


//------------------------------------------------------------------------------------------------
/**
 * Generate a texture for the RectangularDetector
 *
 * @param image_data :: pointer to where the image data will be filled in.
 * @param list :: Color list iterator. Only used if useDetectorIDs is false.
 * @param useDetectorIDs :: set to true to make a fake texture using the detector IDs. If false, the iterator is used.
 *
 */
int RectangularDetectorActor::genTexture(char * & image_data, std::vector<boost::shared_ptr<GLColor> >::iterator& list, bool useDetectorIDs)
{
  int num = mDet->xpixels() * mDet->ypixels();

  //The texture size must be 2^n power.
  int text_x_size, text_y_size;
  mDet->getTextureSize(text_x_size, text_y_size);

  std::cerr << "Texture size: " << text_x_size << ',' << text_y_size <<std::endl;

  //For using color IDs
  int rgb = this->mColorStartID;

  //------ Create the image data buffer -------
  if (!image_data)
  {
    //Delete the old memory
    delete [] image_data;
    image_data = new char[3*text_x_size*text_y_size];
  }
  //Pointer to where we are in it.
  char * image_data_ptr = image_data;

  //Fill with 0 (black) everywhere
  std::fill(image_data, image_data + 3*text_x_size*text_y_size, 0);

  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      //Use black as the default color (for going off the edges)
      char r, g, b;

      if (useDetectorIDs)
      {
        //Use the rgb int that is incremented for each detector
        r=char(rgb/65536);
        g=char((rgb%65536)/256);
        b=char((rgb%65536)%256);
        rgb++;
      }
      else
      {
        //Get the current color
        float rf, gf, bf, af;
        (*list)->get(rf,gf,bf,af);
        //Convert to bytes
        r = (char) (255*rf);
        g = (char) (255*gf);
        b = (char) (255*bf);
        //Go to the next color
        list++;
      }

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

  if (VERBOSE) std::cout << "RectangularDetectorActor::genTexture() called for " << mDet->getName() << " with " << num << " entries set\n";

  return num;
}


//------------------------------------------------------------------------------------------------
/** Upload the texture to the video card. */
void RectangularDetectorActor::uploadTexture(char * & image_data)
{
  if (!image_data)
    throw std::runtime_error("Empty pointer passed to RectangularDetectorActor::uploadTexture()!");

  //The texture size must be 2^n power.
  int text_x_size, text_y_size;
  mDet->getTextureSize(text_x_size, text_y_size);

  // Set up before uploading a texture
  if (mTextureID > 0)
    glDeleteTextures(1,&mTextureID);
  glGenTextures(1, &mTextureID);          // Create The Texture
  if (VERBOSE) std::cout << mDet->getName() << " is drawing with texture id " << mTextureID << "\n";
  mDet->setTextureID(mTextureID);

  glBindTexture(GL_TEXTURE_2D, mTextureID);

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

}

//
///** Swaps between the image_data and pick_data in order
// * to swap the texture between the real one and the pick-color-one.
// */
//void RectangularDetectorActor::swapTexture()
//{
//  char * temp = pick_data;
//  pick_data = image_data;
//  image_data = temp;
//}



//------------------------------------------------------------------------------------------------
/**
 * This method draws the children with the ColorID rather than the children actual color. used in picking the component
 */
void RectangularDetectorActor::drawUsingColorID()
{
  std::vector<boost::shared_ptr<GLColor> >::iterator dummy_iterator;
  this->genTexture(pick_data, dummy_iterator, true);
  this->uploadTexture(pick_data);

  //And now draw it with the newly defined texture.
  this->define();

  //Re-upload the good data from before.
  this->uploadTexture(image_data);
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

/**
  * Call the DetectorCallback::callback method for each detector
  */
void RectangularDetectorActor::detectorCallback(DetectorCallback* callback)const
{
  //The texture size must be 2^n power.
  int text_x_size, text_y_size;
  mDet->getTextureSize(text_x_size, text_y_size);
  //Pointer to where we are in it.
  char * image_data_ptr = image_data;
  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      char r, g, b;
      //Read the color data from the buffer
      r = *image_data_ptr;
      image_data_ptr++;
      g = *image_data_ptr;
      image_data_ptr++;
      b = *image_data_ptr;
      image_data_ptr++;
      if (r == 0 && g == 0 && b == 0)
      {
        std::cerr << "black\n";
      }
      boost::shared_ptr<const Mantid::Geometry::IDetector> det =
        boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector> (mDet->getAtXY(x,y));
      if (det)
      {
        callback->callback(det,DetectorCallbackData(GLColor(float(r)/255,float(g)/255,float(b)/255,1.0)));
      }
    }
    //Skip any padding in x. 3 bytes per pixel
    image_data_ptr += 3*(text_x_size-mDet->xpixels());
  }
  
}

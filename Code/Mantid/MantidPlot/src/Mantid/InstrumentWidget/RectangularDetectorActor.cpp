#include "RectangularDetectorActor.h"
#include "ObjComponentActor.h"
#include "RectangularDetectorActor.h"
#include "InstrumentActor.h"

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;

static const bool VERBOSE = false;


/**
 * Constructor.
 *
 * @param instrActor :: the instrument actor
 * @param compID :: the component ID
 */
RectangularDetectorActor::RectangularDetectorActor(const InstrumentActor& instrActor, const Mantid::Geometry::ComponentID& compID)
:
    ICompAssemblyActor(instrActor,compID), mTextureID(0)
{
  mNumberOfDetectors = 0;
  mDet = boost::dynamic_pointer_cast<const RectangularDetector>(getComponent());
  image_data = NULL;
  pick_data = NULL;

  if (!mDet) return;
  
  BoundingBox compBox;
  mDet->getBoundingBox(compBox);
  mNumberOfDetectors =  mDet->xpixels() * mDet->ypixels();
  this->AppendBoundingBox(compBox.minPoint(), compBox.maxPoint());  

  std::vector<GLColor> clist;
  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      IDetector_const_sptr pixel = mDet->getAtXY(x,y);
      detid_t id = pixel->getID();
      size_t pickID = instrActor.push_back_detid(id, pixel->getPos());
//      // Getting the detector is slow. Get the ID directly
//      detid_t id = mDet->getDetectorIDAtXY(x,y);
//      size_t pickID = instrActor.push_back_detid(id, mDet->get);
      m_pickIDs.push_back(pickID);
      clist.push_back(instrActor.getColor(id));
    }
  }

  genTexture(image_data,clist,false);
  genTexture(pick_data, clist, true);
  uploadTexture(image_data);

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
void RectangularDetectorActor::draw(bool picking)const
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

  //glBindTexture(GL_TEXTURE_2D, mTextureID);
  if (picking)
  {
    this->uploadTexture(pick_data);
  }
  else
  {
    this->uploadTexture(image_data);
  }
  // RectangularDetector will use.
  mDet->draw();
  glBindTexture(GL_TEXTURE_2D, 0);

  glPopMatrix();
}

//------------------------------------------------------------------------------------------------
/**
 * Accept a visitor.
 *
 * @param visitor :: A visitor.
 *
 */
bool RectangularDetectorActor::accept(const GLActorVisitor& visitor)
{
  if (visitor.visit(this)) return true;

  const SetVisibleComponentVisitor* svv = dynamic_cast<const SetVisibleComponentVisitor*>(&visitor);
  if (svv)
  {
    Mantid::Geometry::ComponentID id = svv->getID();
    for (int y=0; y < mDet->ypixels(); y++)
    {
      for (int x=0; x < mDet->xpixels() ; x++)
      {
        if (id == mDet->getAtXY(x,y)->getComponentID())
        {
          setVisibility(true);
          return true;
        }
      }
    }
  }
  return false;
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
int RectangularDetectorActor::genTexture(char * & image_data, std::vector<GLColor>& list, bool useDetectorIDs)
{
  int num = mDet->xpixels() * mDet->ypixels();

  //The texture size must be 2^n power.
  int text_x_size, text_y_size;
  mDet->getTextureSize(text_x_size, text_y_size);

  //std::cerr << "Texture size: " << text_x_size << ',' << text_y_size <<std::endl;

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

  //For using color IDs
  int rgb = 0;
  std::vector<GLColor>::iterator list_it = list.begin();

  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      //Use black as the default color (for going off the edges)
      unsigned char r, g, b;

      if (useDetectorIDs)
      {
        GLColor c = GLActor::makePickColor(m_pickIDs[rgb]);
        c.get(r,g,b);
        rgb++;
      }
      else
      {
        //Get the current color
        list_it->get(r,g,b);
        //Go to the next color
        ++list_it;
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
void RectangularDetectorActor::uploadTexture(char * & image_data)const
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
  //mDet->setTextureID(mTextureID);

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

//-------------------------------------------------------------------------------------------------
/**
 * Return the bounding box, from the one calculated in the cache previously.
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void RectangularDetectorActor::getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const
{
  minBound=minBoundBox;
  maxBound=maxBoundBox;
}


/**
 * Append the bounding box CompAssembly bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void RectangularDetectorActor::AppendBoundingBox(const Mantid::Kernel::V3D& minBound,const Mantid::Kernel::V3D& maxBound)
{
  if(minBoundBox[0]>minBound[0]) minBoundBox[0]=minBound[0];
  if(minBoundBox[1]>minBound[1]) minBoundBox[1]=minBound[1];
  if(minBoundBox[2]>minBound[2]) minBoundBox[2]=minBound[2];
  if(maxBoundBox[0]<maxBound[0]) maxBoundBox[0]=maxBound[0];
  if(maxBoundBox[1]<maxBound[1]) maxBoundBox[1]=maxBound[1];
  if(maxBoundBox[2]<maxBound[2]) maxBoundBox[2]=maxBound[2];
}

void RectangularDetectorActor::setColors()
{
  std::vector<GLColor> clist;
  for (int y=0; y < mDet->ypixels(); y++)
  {
    for (int x=0; x < mDet->xpixels() ; x++)
    {
      detid_t id = mDet->getDetectorIDAtXY(x,y);
      clist.push_back(m_instrActor.getColor(id));
    }
  }
  genTexture(image_data,clist,false);
  uploadTexture(image_data);
}




#include "GLGroupPickBox.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "GLActor.h"
#include "MantidGeometry/V3D.h"
#include "MantidObject.h"
#include "GLActorCollection.h"
#include <QMessageBox>

GLGroupPickBox::GLGroupPickBox()
{
	mBoxStartPtX=0;
	mBoxStartPtY=0;
	mBoxEndPtX=0;
	mBoxEndPtY=0;
	mPickingActive=false;
}

GLGroupPickBox::~GLGroupPickBox()
{
}


/**
 * This methods sets the display image and picker image. display image is used for displaying on the render
 * which is different from the image used for picking using mouse.
 * @param displayImage: input for the display image
 * @param pickerImage: input for the pick image
 */
void GLGroupPickBox::setImages(QImage displayImage,QImage pickerImage)
{
	mDisplayImage=displayImage;
	mPickImage=pickerImage;
}

/**
 * This method sets the display image.
 * @param displayImage: input for the display image
 */
void GLGroupPickBox::setDisplayImage(QImage displayImage)
{
	mDisplayImage=displayImage;
}

/**
 * This method sets the pick image.
 * @param pickerImage: input for the pick image
 */
void GLGroupPickBox::setPickImage(QImage pickerImage)
{
	mPickImage=pickerImage;
}

/**
 * This is the slot for the mouse move operation.
 * when mouse is moved with the button pressed then the picker box grows.
 * @param event: input with mouse event information such as button state
 */ 
void GLGroupPickBox::mouseMoveEvent ( QMouseEvent * event )
{
  if(mPickingActive)
  {
    mBoxEndPtX=event->x();
    mBoxEndPtY=event->y();		
  }
}

/**
 * This is the slot for the mouse press event operation
 * when mouse button is pressed that point will be the start point of the pick box
 * @param event: input with mouse event information such as button state
 */
void GLGroupPickBox::mousePressed(Qt::MouseButtons buttons, const QPoint & pos)
{
  if( (buttons & Qt::LeftButton) || (buttons & Qt::RightButton) )
  {
    mPickingActive = true;
    mBoxStartPtX=pos.x();
    mBoxStartPtY=pos.y();
    mBoxEndPtX=pos.x();
    mBoxEndPtY=pos.y();
  }
  else
  {
    mPickingActive = false;
  }
}

/**
 * This is the slot for the mouse release event operation
 * when the mouse button is released then that point will be the end point of the pick box
 * @param event: input with mouse event information such as button state
 */
void GLGroupPickBox::mouseReleased (Qt::MouseButtons buttons, const QPoint & pos )
{
  (void) buttons; //avoid compiler warning

  if(mPickingActive)
  {
    mPickingActive=false;
    mBoxEndPtX=pos.x();
    mBoxEndPtY=pos.y();	
    //Lookup in the actors collection which have same pixel color value as the
    //pixels in the box, collect unique colors
    std::set<QRgb> colorSet;
    //Check for the box limits
    int x,y,width,height;
    x=mBoxStartPtX;
    y=mBoxStartPtY;
    width=mBoxEndPtX-mBoxStartPtX;
    height=mBoxEndPtY-mBoxStartPtY;
    if(width<0)
    {
      x=mBoxEndPtX;
      width*=-1;
    }
    else if(width==0)
    {
      width=1;
    }
    if(height<0)
    {
      y=mBoxEndPtY;
      height*=-1;
    }
    else if(height==0)
    {
      height=1;
    }
    if (height > 1 && width > 1)
    {
      QImage selectedImage=mPickImage.copy(x,y,width,height);
      for(int iPix=0;iPix<selectedImage.width();iPix++)
      {
        for(int jPix=0;jPix<selectedImage.height();jPix++)
        {
          QRgb colorVal=selectedImage.pixel(iPix,jPix);
          colorSet.insert(colorVal);
        }
      }
    }
    mColorSet=colorSet;
  }
}

/**
 * Returns the list of colors picked
 */
std::set<QRgb> GLGroupPickBox::getListOfColorsPicked()
{
	return mColorSet;
}

/**
 * picking a actor at a input point from image
 * @param x :: input x-dim value of the point
 * @param y :: input y-dim value of the point
 */
QRgb GLGroupPickBox::pickPoint(int x, int y) // Picking object at coordinate of (x,y)
{
  if( mPickImage.valid(x, y) ) 
  {
    return mPickImage.pixel(x,y);
  }
  else return QRgb();
}

/**
 * This method draws the display image along with the box
 */
void GLGroupPickBox::draw(QPainter* painter)
{
    painter->drawImage(0,0,mDisplayImage);
    if (mBoxEndPtX != mBoxStartPtX && mBoxEndPtY != mBoxStartPtY)
    {
      drawPickBox(painter);
    }
  }

/**
 * This method draws the pick box
 */
void GLGroupPickBox::drawPickBox(QPainter* painter)
{
    painter->setPen(Qt::blue);
    painter->drawRect(mBoxStartPtX,mBoxStartPtY,mBoxEndPtX-mBoxStartPtX,mBoxEndPtY-mBoxStartPtY);
}

void GLGroupPickBox::hide()
{
  mBoxEndPtX = mBoxStartPtX;
  mBoxEndPtY = mBoxStartPtY;
}

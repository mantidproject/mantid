#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/Detector.h"

#include <algorithm>
#include <stdexcept> 
#include <ostream>
namespace Mantid
{
namespace Geometry
{



/*! Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os  :: output stream
 * @param ass :: component assembly
 *
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 *  Also output the number of children
 */
std::ostream& operator<<(std::ostream& os, const RectangularDetector& ass)
{
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}


/*! Empty constructor
 */
RectangularDetector::RectangularDetector() : CompAssembly()
{
}

/*! Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
RectangularDetector::RectangularDetector(const std::string& n, Component* reference) :
    CompAssembly(n, reference)
{
}

/*! Copy constructor
 *  @param other :: RectangularDetector to copy
 */
RectangularDetector::RectangularDetector(const RectangularDetector& other) :
  CompAssembly(other)
{
  //TODO: Copy other fields here
}

/*! Destructor
 */
RectangularDetector::~RectangularDetector()
{

}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* RectangularDetector::clone() const
{
  return new RectangularDetector(*this);
}



/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position.
 *
 * @param X index from 0..xPixels-1
 * @param Y index from 0..yPixels-1
 * @throws runtime_error if the x/y pixel width is not set, or X/Y are out of range
 */
boost::shared_ptr<Detector> RectangularDetector::getAtXY(int X, int Y) const
{
  if ((xPixels <= 0) || (yPixels <= 0))
  {
    //std::cout << "xPixels " << xPixels << " yPixels " << yPixels << "\n";
    throw std::runtime_error("RectangularDetector::getAtXY: invalid X or Y width set in the object.");
  }
  if ((X < 0) || (X >= xPixels))
    throw std::runtime_error("RectangularDetector::getAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= yPixels))
    throw std::runtime_error("RectangularDetector::getAtXY: Y specified is out of range.");
  //Find the index and return that.
  int i = X*yPixels + Y;
  return boost::dynamic_pointer_cast<Detector>( this->operator[](i) );
}

/// Returns the number of pixels in the X direction.
int RectangularDetector::xpixels() const
{
  return this->xPixels;
}

/// Returns the number of pixels in the X direction.
int RectangularDetector::ypixels() const
{
  return this->yPixels;
}



/** Initialize a RectangularDetector by creating all of the pixels
 * contained within it. You should have set the name, position
 * and rotation and facing of this object BEFORE calling this.
 *
 * @param shape a geometry Object containing the shape of the pixel
 * @param xpixels number of pixels in X
 * @param xstart x-position of the 0-th pixel (in length units, normally meters)
 * @param xstep step size between pixels in the horizontal direction (in length units, normally meters)
 * @param ypixels number of pixels in Y
 * @param ystart y-position of the 0-th pixel (in length units, normally meters)
 * @param ystep step size between pixels in the vertical direction (in length units, normally meters)
 * @param idstart detector ID of the first pixel
 * @param idfillbyfirst_y set to true if ID numbers increase with Y indices first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idstepbyrow amount to increase the ID number on each row. e.g, if you fill by Y first,
 *            and set  idstepbyrow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 *
 */
void RectangularDetector::initialize(boost::shared_ptr<Object> shape,
    int xpixels, double xstart, double xstep,
    int ypixels, double ystart, double ystep,
    int idstart, bool idfillbyfirst_y, int idstepbyrow
    )
{
  xPixels = xpixels;
  yPixels = ypixels;
  //TODO: some safety checks

  std::string name = this->getName();

  //Loop through all the pixels
  int ix, iy;
  for (ix=0; ix<xPixels; ix++)
    for (iy=0; iy<yPixels; iy++)
    {
      //Make the name
      std::ostringstream oss;
      oss << name << "(" << ix << "," << iy << ")";

      //Create the detector from the given shape and with THIS as the parent.
      Detector* detector = new Detector(oss.str(), shape, this);

      //Calculate its id and set it.
      int id;
      if (idfillbyfirst_y)
        id = idstart + ix * idstepbyrow + iy;
      else
        id = idstart + iy * idstepbyrow + ix;
      detector->setID(id);

      //Calculate the x,y position
      double x = xstart + ix * xstep;
      double y = ystart + iy * ystep;
      V3D pos(x,y,0);
      //Translate (relative to parent)
      detector->translate(pos);

      //Add it to this assembly
      this->add(detector);

    }

}


} // Namespace Geometry
} // Namespace Mantid


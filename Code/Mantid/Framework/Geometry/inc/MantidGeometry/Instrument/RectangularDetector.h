#ifndef RECTANGULAR_DETECTOR_H
#define RECTANGULAR_DETECTOR_H
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IObjComponent.h"

namespace Mantid
{
namespace Geometry
{
/**
 *  RectangularDetector is a type of CompAssembly, an assembly of components.
 *  It is designed to be an easy way to specify a rectangular (XY) array of
 *  Detector pixels.
 *
 * @class RectangularDetector
 * @brief Assembly of Detector objects in a rectangular shape
 * @author Janik Zikovsky, SNS
 * @date 2010-Oct-06

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class DLLExport RectangularDetector : public CompAssembly, public IObjComponent
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "RectangularDetector";}
  //! Empty constructor
  RectangularDetector();

  //! Constructor with a name and parent reference
  RectangularDetector(const std::string&, IComponent* reference=0);

  //! Parametrized constructor
  RectangularDetector(const RectangularDetector* base, const ParameterMap * map);

  /// Create all the detector pixels of this rectangular detector.
  void initialize(boost::shared_ptr<Object> shape,
      int xpixels, double xstart, double xstep,
      int ypixels, double ystart, double ystep,
      int idstart, bool idfillbyfirst_y, int idstepbyrow,
      int idstep=1
      );

//  //! Copy constructor
//  RectangularDetector(const RectangularDetector&);
  virtual ~RectangularDetector();
  //! Make a clone of the present component
  virtual IComponent* clone() const;

  /// Get a detector at given XY indices.
  boost::shared_ptr<Detector> getAtXY(const int X, const int Y) const;

  std::pair<int, int> getXYForDetectorID(const int detectorID) const;

  int xpixels() const;
  int ypixels() const;
  
  double xstep() const;
  double ystep() const;

  double xstart() const;
  double ystart() const;

  ///Size in X of the detector
  double xsize() const;
  ///Size in Y of the detector
  double ysize() const;

  V3D getRelativePosAtXY(int x, int y) const;
  void getTextureSize(int & xsize, int & ysize) const;

  unsigned int getTextureID() const;
  void setTextureID(unsigned int textureID);
  ///minimum detector id
  int minDetectorID();
  /// maximum detector id
  int maxDetectorID();

  // This should inherit the getBoundingBox implementation from  CompAssembly but
  // the multiple inheritance seems to confuse it so we'll explicityly tell it that here
  using CompAssembly::getBoundingBox;

  virtual void testIntersectionWithChildren(Track & testRay, std::deque<IComponent_sptr> & searchQueue) const;

  // ------------ IObjComponent methods ----------------

  /// Does the point given lie within this object component?
  bool isValid(const V3D& point) const ;

  /// Does the point given lie on the surface of this object component?
  bool isOnSide(const V3D& point) const ;

  ///Checks whether the track given will pass through this Component.
  int interceptSurface(Track& track) const ;

  /// Finds the approximate solid angle covered by the component when viewed from the point given
  double solidAngle(const V3D& observer) const;
  /// Retrieve the cached bounding box
  void getBoundingBox(BoundingBox & assemblyBox) const;

  ///Try to find a point that lies within (or on) the object
  int getPointInObject(V3D& point) const;

  //Rendering member functions
  ///Draws the objcomponent.
  void draw() const;

  /// Draws the Object.
  void drawObject() const;

  /// Initializes the ObjComponent for rendering, this function should be called before rendering.
  void initDraw() const;

  /// Returns the shape of the Object
  const boost::shared_ptr<const Object> shape() const;
  /// Returns the material of the detector
  const boost::shared_ptr<const Material> material() const
  {
    return boost::shared_ptr<const Material>();
  }

  // ------------ End of IObjComponent methods ----------------


private:
  /// Pointer to the base RectangularDetector, for parametrized instruments
  const RectangularDetector * m_rectBase;
  /// Private copy assignment operator
  RectangularDetector& operator=(const ICompAssembly&);

  /// The number of pixels in the X (horizontal) direction;
  int m_xpixels;
  /// The number of pixels in the Y (vertical) direction;
  int m_ypixels;

  /// Size in X of the detector
  double m_xsize;
  /// Size in Y of the detector
  double m_ysize;

  /// X position of the 0-th pixel
  double m_xstart;
  /// Y position of the 0-th pixel
  double m_ystart;

  /// Step size in the X direction of detector
  double m_xstep;
  /// Step size in the Y direction of detector
  double m_ystep;

  /// Texture ID to use in rendering
  unsigned int mTextureID;

  /// Pointer to the shape of the pixels in this detector array.
  boost::shared_ptr<Object> mShape;
  /// minimum detector id
  int m_minDetId;
  /// maximum detector id
  int m_maxDetId;

  /// IDs start here
  int m_idstart;
  /// IDs are filled in Y fastest
  bool m_idfillbyfirst_y;
  /// Step size in ID in each row
  int m_idstepbyrow;
  /// Step size in ID in each col
  int m_idstep;

};

DLLExport std::ostream& operator<<(std::ostream&, const RectangularDetector&);

typedef boost::shared_ptr<RectangularDetector> RectangularDetector_sptr;
typedef boost::shared_ptr<const RectangularDetector> RectangularDetector_const_sptr;

} //Namespace Geometry
} //Namespace Mantid

#endif

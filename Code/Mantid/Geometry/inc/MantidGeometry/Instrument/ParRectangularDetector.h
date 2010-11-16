#ifndef PARRECTANGULARDETECTOR_H
#define PARRECTANGULARDETECTOR_H
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IRectangularDetector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

namespace Mantid
{
namespace Geometry
{

class CompAssembly;

/** @class ParRectangularDetector
    @brief Parametrized RectangularDetector
    @author Janik Zikovsky, SNS

    Note: This class hierarchy will get cleaned up. I know it is ugly as it
    is, but for now, just trying to make things work. 2010-10-25.

    
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
class DLLExport ParRectangularDetector : public CompAssembly, public IRectangularDetector, public IObjComponent
{
public:
  /// Constructors
  ParRectangularDetector(const RectangularDetector* base, const ParameterMap * map);
  ParRectangularDetector(const ParRectangularDetector & other);

  //! Destructor
  ~ParRectangularDetector(){}

  ///String description of the type of component
  virtual std::string type() const { return "ParRectangularDetector";}

  IComponent* clone() const;


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
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
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

  // ------------ End of IObjComponent methods ----------------



  /// Get a detector at given XY indices.
  boost::shared_ptr<Detector> getAtXY(int X, int Y) const;
  /// Return the number of pixels in the X direction
  int xpixels() const;
  /// Return the number of pixels in the Y direction
  int ypixels() const;

  double xstep() const;
  double ystep() const;

  ///Size in X of the detector
  double xsize() const;
  ///Size in Y of the detector
  double ysize() const;
  void getTextureSize(int & xsize, int & ysize) const;

  V3D getRelativePosAtXY(int x, int y) const;

private:
  /// Private copy assignment operator
  //ParRectangularDetector& operator=(const ICompAssembly&);
  const RectangularDetector * mBase;

  /// A cached bounding box
  mutable BoundingBox *m_cachedBoundingBox;


};

//DLLExport std::ostream& operator<<(std::ostream&, const CompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif

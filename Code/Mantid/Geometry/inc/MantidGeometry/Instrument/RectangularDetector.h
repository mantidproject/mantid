#ifndef RECTANGULAR_DETECTOR_H
#define RECTANGULAR_DETECTOR_H
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Objects/Object.h"

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

class DLLExport RectangularDetector : public CompAssembly
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "RectangularDetector";}
  //! Empty constructor
  RectangularDetector();

  //! Constructor with a name and parent reference
  RectangularDetector(const std::string&, Component* reference=0);

  /// Create all the detector pixels of this rectangular detector.
  void initialize(boost::shared_ptr<Object> shape,
      int xpixels, double xstart, double xstep,
      int ypixels, double ystart, double ystep,
      int idstart, bool idfillbyfirst_y, int idstepbyrow
      );

  //! Copy constructor
  RectangularDetector(const RectangularDetector&);
  virtual ~RectangularDetector();
  //! Make a clone of the present component
  virtual IComponent* clone() const;

  /// Get a detector at given XY indices.
  boost::shared_ptr<Detector> getAtXY(int X, int Y) const;

  int xpixels() const;
  int ypixels() const;

private:
  /// Private copy assignment operator
  RectangularDetector& operator=(const ICompAssembly&);

  /// The number of pixels in the X (horizontal) direction;
  int xPixels;
  /// The number of pixels in the Y (vertical) direction;
  int yPixels;


};

DLLExport std::ostream& operator<<(std::ostream&, const RectangularDetector&);

} //Namespace Geometry
} //Namespace Mantid

#endif

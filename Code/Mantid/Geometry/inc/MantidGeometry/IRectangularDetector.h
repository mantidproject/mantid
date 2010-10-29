#ifndef IRECTANGULAR_DETECTOR_H
#define IRECTANGULAR_DETECTOR_H
#include <string> 
#include <vector>
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{
/** @class IRectangularDetector
    @brief Interface for RectangularDetector, used by ParRectangularDetector
    @author Janik Zikovsky, SNS

    This class is extremely repetitive of RectangularDetector; for that, I apologize,
    but the Component wrapper mess requires it.

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
class DLLExport IRectangularDetector : public virtual ICompAssembly
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "IRectangularDetector";}

  /// Get a detector at given XY indices.
  virtual boost::shared_ptr<Detector> getAtXY(int X, int Y) const = 0;
  /// Return the number of pixels in the X direction
  virtual int xpixels() const = 0;
  /// Return the number of pixels in the Y direction
  virtual int ypixels() const = 0;

  virtual double xstep() const = 0;
  virtual double ystep() const = 0;
  virtual void getTextureSize(int & xsize, int & ysize) const = 0;

  ///Size in X of the detector
  virtual double xsize() const = 0;
  ///Size in Y of the detector
  virtual double ysize() const = 0;

  virtual V3D getRelativePosAtXY(int x, int y) const = 0;

};

} //Namespace Geometry
} //Namespace Mantid

#endif

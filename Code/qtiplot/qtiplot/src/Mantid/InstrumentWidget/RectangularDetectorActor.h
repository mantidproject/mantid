#ifndef RECTANGULAR_DETECTOR_ACTOR__H_
#define RECTANGULAR_DETECTOR_ACTOR__H_
#include "GLActor.h"
#include "ObjComponentActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IRectangularDetector.h"
#include "MantidGeometry/V3D.h"
/*!
  \class  RectangularDetectorActor
  \brief  This class wraps a RectangularDetector into Actor.
  \author Janik Zikovsky
  \date   October 7 2010
  \version 1.0

  This class is used to render a RectangularDetector as a bitmap and plot it.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
*/
namespace Mantid
{

namespace Geometry
{
  class ICompAssembly;
  class Object;
  class V3D;
}
namespace API
{
  class IInstrument;
}

}

class MantidObject;
class ObjComponentActor;

class RectangularDetectorActor : public ObjComponentActor
{
public:
  /// Constructor
  RectangularDetectorActor(boost::shared_ptr<Mantid::Geometry::IRectangularDetector> rectDet);
  /// Destructor
  virtual ~RectangularDetectorActor();

private:
  int mNumberOfDetectors;          ///< The number of detectors in the Component assembly 
  Mantid::Geometry::V3D minBoundBox;
  Mantid::Geometry::V3D maxBoundBox;
  void AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound);

protected:
  /// Component ID of the RectangularDetector
  Mantid::Geometry::ComponentID mId;
  /// Instrument
  boost::shared_ptr<Mantid::API::IInstrument> mInstrument;

  int mColorStartID;                                       ///< Starting picking colorid for the subcomponents to CompAssembly

  boost::shared_ptr<Mantid::Geometry::IRectangularDetector> mDet;

  void init();
  void redraw();
  int findDetectorIDUsingColor(int rgb);

public:
  virtual std::string type()const {return "RectangularDetectorActor";} ///< Type of the GL object
  void define();  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  void appendObjCompID(std::vector<int>& idList);
  int setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator & list);
  void getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound);
};

#endif /*RECTANGULAR_DETECTOR_ACTOR__H_*/


#ifndef RECTANGULAR_DETECTOR_ACTOR__H_
#define RECTANGULAR_DETECTOR_ACTOR__H_
#include "GLActor.h"
#include "ObjComponentActor.h"
#include "ICompAssemblyActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/V3D.h"
/**
  \class  RectangularDetectorActor
  \brief  This class wraps a RectangularDetector into Actor.
  \author Janik Zikovsky
  \date   October 7 2010
  \version 1.0

  This class is used to render a RectangularDetector as a bitmap and plot it.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
*/
namespace Mantid
{
namespace Kernel
{
  class V3D;
}
namespace Geometry
{
  class ICompAssembly;
  class Object;
}

}

class ObjComponentActor;

class RectangularDetectorActor : public ICompAssemblyActor
{
public:
  /// Constructor
  RectangularDetectorActor(const InstrumentActor& instrActor, const Mantid::Geometry::ComponentID& compID);
  /// Destructor
  virtual ~RectangularDetectorActor();

private:
  void AppendBoundingBox(const Mantid::Kernel::V3D& minBound,const Mantid::Kernel::V3D& maxBound);

protected:
  /// The rectangular detector
  boost::shared_ptr<const Mantid::Geometry::RectangularDetector> mDet;

  void init()const;
  void redraw();
  int findDetectorIDUsingColor(int rgb);
  virtual void initChilds(bool) {}

public:
  virtual std::string type()const {return "RectangularDetectorActor";} ///< Type of the GL object

  void draw(bool picking = false)const;  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;
  bool accept(GLActorVisitor& visitor, VisitorAcceptRule rule = VisitAll);
  bool accept(GLActorConstVisitor& visitor, VisitorAcceptRule rule = VisitAll)const;
  bool isChildDetector(const Mantid::Geometry::ComponentID& id ) const;
  virtual void setColors();

  int genTexture(char * & image_data, std::vector<GLColor>& list, bool useDetectorIDs);
  void uploadTexture(char * & image_data)const;

private:
  /// Texture ID that holds the texture.
  mutable unsigned int mTextureID;

  /// Pointer to the array holding the texture color data
  mutable char * image_data;

  /// Pointer to the array holding the color data for picking the scene
  mutable char * pick_data;

  /// pick ids
  std::vector<size_t> m_pickIDs;
};

#endif /*RECTANGULAR_DETECTOR_ACTOR__H_*/


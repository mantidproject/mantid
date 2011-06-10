#ifndef RECTANGULAR_DETECTOR_ACTOR__H_
#define RECTANGULAR_DETECTOR_ACTOR__H_
#include "GLActor.h"
#include "ObjComponentActor.h"
#include "ICompAssemblyActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/V3D.h"
/**
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

class RectangularDetectorActor : public ICompAssemblyActor //ObjComponentActor
{
public:
  /// Constructor
  RectangularDetectorActor(const InstrumentActor& instrActor, const Mantid::Geometry::ComponentID& compID);
  /// Destructor
  virtual ~RectangularDetectorActor();

private:
  void AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound);

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
  void getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)const;
  virtual void setColors();

  void appendObjCompID(std::vector<int>& idList);
  int setInternalDetectorColors(std::vector<GLColor>::iterator & list);
  int genTexture(char * & image_data, std::vector<GLColor>::iterator& list, bool useDetectorIDs);
  void uploadTexture(char * & image_data)const;

  virtual int  setStartingReferenceColor(int rgb);
  virtual void drawUsingColorID();

  virtual MantidObject* getMantidObject(const boost::shared_ptr<const Mantid::Geometry::Object>, bool)
  {
    return NULL;
  }

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


#ifndef ICOMPASSEMBLY_ACTOR__H_
#define ICOMPASSEMBLY_ACTOR__H_
#include "GLActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"

#include "boost/shared_ptr.hpp"

#include <map>

/*!
  \class  ICompAssemblyActor
  \brief  This class wraps the ICompAssembly into Actor.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

  This class has the interface Comp assembly actors.

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
  class IInstrument;
}

}

class MantidObject;
class ObjComponentActor;

class ICompAssemblyActor : public GLActor
{
protected:
  int mNumberOfDetectors;
  Mantid::Geometry::V3D minBoundBox;
  Mantid::Geometry::V3D maxBoundBox;
  //void AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound);

  Mantid::Geometry::ComponentID mId; ///< Component ID of the CompAssembly
  boost::shared_ptr<Mantid::Geometry::IInstrument> mInstrument; ///< Instrument
  boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> > mObjects; ///< List of Objects in the Instrument, its built as the Instrument is parsed through
  int mColorStartID;                                       ///< Starting picking colorid for the subcomponents to CompAssembly
  virtual void initChilds(bool) = 0;
public:
  ICompAssemblyActor(bool withDisplayList);                       ///< Constructor
  ICompAssemblyActor(boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> >& mmm,
                     Mantid::Geometry::ComponentID id,
                     boost::shared_ptr<Mantid::Geometry::IInstrument> ins,
                     bool withDisplayList); ///< Constructor
  virtual ~ICompAssemblyActor(){}								   ///< Destructor
  virtual int  setStartingReferenceColor(int rgb) = 0;
  virtual std::string type()const {return "ICompAssemblyActor";} ///< Type of the GL object
  //virtual void define() = 0;  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  const int  getNumberOfDetectors() const{return mNumberOfDetectors;}
  virtual void drawUsingColorID() = 0;
  void getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound);

  //virtual void init() = 0;
  virtual void redraw() = 0;
  virtual void appendObjCompID(std::vector<int>&) = 0;
  virtual MantidObject*	getMantidObject(const boost::shared_ptr<const Mantid::Geometry::Object>,bool withDisplayList) = 0;
  virtual int setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator list){return 0;}// = 0;
  virtual int findDetectorIDUsingColor(int rgb) = 0;

};

#endif /*GLTRIANGLE_H_*/


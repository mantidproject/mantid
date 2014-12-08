#ifndef COMPASSEMBLY_ACTOR__H_
#define COMPASSEMBLY_ACTOR__H_

#include "ICompAssemblyActor.h"
#include "GLActor.h"

#include "MantidGeometry/IComponent.h"
#include "MantidKernel/V3D.h"
/**
  \class  CompAssemblyActor
  \brief  This class wraps the ICompAssembly into Actor.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

  This class has the implementation for calling the children of ICompAssembly's IObjComponent to render themselves
  and call the ICompAssemblys. This maintains the count of the children for easy lookup.

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

class CompAssemblyActor : public ICompAssemblyActor
{
public:
  CompAssemblyActor(const InstrumentActor& instrActor,const Mantid::Geometry::ComponentID& compID); ///< Constructor
  virtual ~CompAssemblyActor();					
  virtual std::string type()const {return "CompAssemblyActor";} ///< Type of the GL object
  virtual void draw(bool picking = false)const;  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  void setChildVisibility(bool);
  bool hasChildVisible() const;
  bool accept(GLActorVisitor& visitor, VisitorAcceptRule rule = VisitAll);
  bool accept(GLActorConstVisitor& visitor, VisitorAcceptRule rule = VisitAll)const;
  virtual void setColors();

protected:
  mutable std::vector<ObjComponentActor*> mChildObjCompActors;     ///< List of ObjComponent Actors
  mutable std::vector<ICompAssemblyActor*> mChildCompAssemActors;   ///< List of CompAssembly Actors
private:
  void AppendBoundingBox(const Mantid::Kernel::V3D& minBound,const Mantid::Kernel::V3D& maxBound);
};

#endif /*GLTRIANGLE_H_*/


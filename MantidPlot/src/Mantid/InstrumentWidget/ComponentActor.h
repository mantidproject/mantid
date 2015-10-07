#ifndef COMPONENT_ACTOR_H_
#define COMPONENT_ACTOR_H_
#include "GLActor.h"
#include "GLColor.h"

#include "MantidGeometry/IComponent.h"

/**
  \class  ObjComponentActor
  \brief  ObjComponentActor is an actor class for rendering ObjComponents.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

   This class has the implementation for rendering ObjComponents in OpenGL and it inherits from the GLActor

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
  namespace Geometry
  {
    class IObjComponent;
    class IDetector;
    class ObjCompAssembly;
    class CompAssembly;
  }
}

class InstrumentActor;

class ComponentActor : public GLActor
{
public:
  ComponentActor(const InstrumentActor& instrActor,const Mantid::Geometry::ComponentID& compID); ///< Default Constructor
  virtual std::string type()const {return "ComponentActor";} ///< Type of the GL object
  bool accept(GLActorVisitor& visitor, VisitorAcceptRule rule = VisitAll);
  bool accept(GLActorConstVisitor& visitor, VisitorAcceptRule rule = VisitAll)const;
  boost::shared_ptr<const Mantid::Geometry::IComponent> getComponent() const;
  boost::shared_ptr<const Mantid::Geometry::IObjComponent> getObjComponent() const;
  boost::shared_ptr<const Mantid::Geometry::IDetector> getDetector() const;
  boost::shared_ptr<const Mantid::Geometry::ObjCompAssembly> getObjCompAssembly() const;
  boost::shared_ptr<const Mantid::Geometry::CompAssembly> getCompAssembly() const;
  virtual void setColors(){}
  /// Check if the component is a non-detector.
  bool isNonDetector() const;
protected:
  const InstrumentActor& m_instrActor;
  Mantid::Geometry::ComponentID m_id; ///< Component ID
};

#endif /*COMPONENT_ACTOR_H_*/


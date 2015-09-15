#ifndef OBJCOMPONENT_ACTOR_H_
#define OBJCOMPONENT_ACTOR_H_
#include "ComponentActor.h"
#include "GLColor.h"
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
namespace Mantid{
  namespace Kernel
  {
    class V3D;
  }

  namespace Geometry
  {
		class IObjComponent;
	}
}
class InstrumentActor;

class ObjComponentActor : public ComponentActor
{
public:
  ObjComponentActor(const InstrumentActor& instrActor,Mantid::Geometry::ComponentID compID); ///< Default Constructor
  ~ObjComponentActor();								   ///< Destructor
  virtual std::string type()const {return "ObjComponentActor";} ///< Type of the GL object
  virtual void draw(bool picking = false)const;  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  virtual void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;
  virtual void setColors();

  void setColor(const GLColor& c){m_dataColor = c;}

private:
  void setPickColor(const GLColor& c){m_pickColor = c;}

  GLColor m_dataColor;
  GLColor m_pickColor;

  friend class InstrumentActor;
};

#endif /*OBJCOMPONENT_ACTOR_H_*/


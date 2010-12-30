#ifndef MANTIDOBJECT_H_
#define MANTIDOBJECT_H_
#include "GLObject.h"
/*!
  \class  MantidObject
  \brief  Mantid Objent wrapper class for rendering in OpenGL
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

   This class has the implementation for rendering OpenGL object and it inherits from the GLObject

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
namespace Mantid{
	namespace Geometry{
		class Object;
	}
}

class MantidObject : public GLObject
{
protected:
  boost::shared_ptr<const Mantid::Geometry::Object> Obj; ///< Holder for Object
  bool mHighResolution;
public:
  MantidObject(const boost::shared_ptr<const Mantid::Geometry::Object> obj,bool withDisplayList=true); ///< Default Constructor
  ~MantidObject();								   ///< Destructor
  virtual std::string type()const {return "MantidObject";} ///< Type of the GL object
  void define();  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  const boost::shared_ptr<const Mantid::Geometry::Object> getObject(); ///< Returns the objcomponent held in this object
  void defineBoundingBox();
  void setResolutionToHigh();
  void setResolutionToLow();
};

#endif /*GLTRIANGLE_H_*/


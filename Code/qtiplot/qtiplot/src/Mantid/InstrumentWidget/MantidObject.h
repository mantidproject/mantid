#ifndef MANTIDOBJECT_H_
#define MANTIDOBJECT_H_
#include "GLObject.h" 
/*!
  \class  MantidObject
  \brief  Mantid ObjComponent OpenGL GLObject class
  \author Srikanth Nagella
  \date   August 2008
  \version 1.0

   This is Concrete GLObject class which implements define method for ObjComponenet.

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
class Mantid::Geometry::ObjComponent;

class MantidObject : public GLObject
{
private:
	Mantid::Geometry::ObjComponent *Obj; ///< Holder for ObjComponenet
public:
	MantidObject(Mantid::Geometry::ObjComponent* obj); ///< Default Constructor
	~MantidObject();								   ///< Destructor
	virtual std::string type()const {return "MantidObject";} ///< Type of the GL object
    void define();  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
	Mantid::Geometry::ObjComponent* getComponent(); ///< Returns the objcomponent held in this object
};

#endif /*GLTRIANGLE_H_*/


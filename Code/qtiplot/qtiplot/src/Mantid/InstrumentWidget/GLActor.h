/*!________________________________________________
* Library        : NTK
* Name           : GLActor.h
* Author         : L.C.Chapon
* Date           : 8 Nov 2006
* Description    : Base class for all objects in a 3D Scene.
*                  Methods are provide to position and
*                  rotate the objects. The objects can also
*                  be set as active or not. Actors maintian safe pointer
*                  to a GLObject.
*________________________________________________
*/
#ifndef GLACTOR_H_
#define GLACTOR_H_
#include "MantidGeometry/V3D.h"
#include "GLObject.h"
#include "GLColor.h"
#include "boost/shared_ptr.hpp"
#include <ostream>

/*!
  \class  GLActor
  \brief  An actor class that holds geometry objects with its position.
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  Base class for all objects in a 3D Scene. Methods are provide to position and rotate the objects.
  The objects can also be set as active or not. Actors maintian safe pointer to a GLObject.

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
class GLActor
{
public:
	GLActor(char* name=0);          ///< Constructor with name of actor as input string
	GLActor(const GLActor&);        ///< Constructor with another actor as input
	virtual ~GLActor();             ///< Virtual destructor
    //! This the base function to draw.
    void draw();
    void drawBoundingBox();
	void getBoundingBox(Mantid::Geometry::V3D& minPoint,Mantid::Geometry::V3D& maxPoint);
    void drawIDColor();
    void setPos(double, double, double);
    void translate(double,double,double);
    void setColorID(unsigned char, unsigned char, unsigned char);
    void setColor(boost::shared_ptr<GLColor>);
    void setRepresentation(boost::shared_ptr<GLObject>);
	boost::shared_ptr<GLObject> getRepresentation();
    void markPicked();
    void markUnPicked();
    friend std::ostream& operator<<(std::ostream& os,const GLActor& a)
    {
        os << "Actor Name:" << a._name << std::endl;
        return os;
    } ///< Printing Actor object
protected:
	boost::shared_ptr<GLObject> _representation; ///< A place holder for the geometry object
	Mantid::Geometry::V3D* _pos;     ///< Position of the geometry object
private:
    boost::shared_ptr<GLColor> _color;           ///< Color of the geometry object
    char* _name;                     ///< Name given to the actor
    bool  _picked;                   ///< Flag Whether the actor is picked by mouse click or not
	unsigned char _colorID[3];		 ///< ColorID for picking. This is assigned by a collection.
};

#endif /*GLACTOR_H_*/

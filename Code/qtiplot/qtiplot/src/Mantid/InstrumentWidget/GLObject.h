#ifndef GLOBJECT_H_
#define GLOBJECT_H_
#include "MantidGeometry/V3D.h"
#include "GLColor.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
/*!
  \class  GLObject
  \brief  Interface for OpenGL object stored in a display list
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

   Concrete GLObject need to overload the "define" function giving OpenGL commands for representing
   the object. The device displaying OpenGL should call the initialization of OpenGL before any
   GLObject is created otherwise glGenLists return systematically 0.

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
class GLObject
{
public:
	GLObject(bool withDisplayList);        ///< Constructor
	virtual ~GLObject();                   ///< Destructor
	virtual std::string type()const {return "GLObject";} ///< Type of the GL object
    void draw();
    virtual void define();
	virtual void init();
protected:
    void construct();
    GLuint mDisplayListId;                   ///< OpengGL Display list id
    bool mChanged;                         ///< Flag holding the change in the object
};
#endif /*GLOBJECT_H_*/


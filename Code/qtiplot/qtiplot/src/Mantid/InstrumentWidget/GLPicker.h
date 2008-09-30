#ifndef GLPICKER_H_
#define GLPICKER_H_
#include "GLColor.h"
#include "GLActorCollection.h"
#include "GLViewport.h"
/*!
  \class  GLPicker
  \brief  class handling picking of GLObjects
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  Class for picking based on color coding. Picking of a single point on the viewport 
  or rectangular area.

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
class GLPicker
{
public:
	GLPicker(GLActorCollection* collection=0);     ///< Constructor
	~GLPicker();                                   ///< Destructor 
    void setPickerColor(float,float,float,float);
    void setPickerColor(const GLColor&);
    void setActorCollection(GLActorCollection*);
    GLActor* pickPoint(int,int);
    void pickAreaStart(int,int);
    void pickAreaFinish(int,int);
    void drawArea(int,int);
    void setViewport(GLViewport*);
private:
    int _rectx1;                   ///< Rectangular box bottom left x value
	int _recty1;                   ///< Rectangular box bottom left y value                       
	int _rectx2;                   ///< Rectangular box top right x value
	int _recty2;                   ///< Rectangular box top right x value
    GLColor _pickingColor;         ///< Picking color
    GLViewport* _viewport;         ///< Viewport
    GLActorCollection* _actors;    ///< Actor collection
};

#endif /*GLPICKER_H_*/

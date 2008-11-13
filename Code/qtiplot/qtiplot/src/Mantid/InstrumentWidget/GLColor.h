#ifndef GLCOLOR_H_
#define GLCOLOR_H_
/*!
  \class  GLColor
  \brief  class handling OpenGL color for objects
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  GLColor class handles the OpenGL color for an object based on the type of the 
  rendering selected. eg. MATERIAL by specifying color as glMaterial rather than
  glColor.

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
class GLColor
{
public:
    enum paintMethod {PLAIN=0,MATERIAL=1,EMIT=2};
	GLColor(float R=0,float G=0,float B=0,float A=0); ///< Constructor
    GLColor(const GLColor&);                          ///< Constructor
	virtual ~GLColor();                               ///< Destructor
    void set(float,float,float,float);
	void get(float&,float&,float&,float&);
    void paint(paintMethod);
private:
    float _v[4];                                      ///< Color Component Values
};

#endif /*GLCOLOR_H_*/


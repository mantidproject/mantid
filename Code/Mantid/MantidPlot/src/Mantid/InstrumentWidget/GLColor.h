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

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  /// Enumeration for painting type
  enum PaintMethod {PLAIN = 0, MATERIAL = 1, EMIT = 2};
  
  /// Default Constructor
  GLColor(float red = 0, float green = 0,float blue = 0, float alpha = 0);
  /// Destructor
  virtual ~GLColor();  

  /// Set all four values atomically
  void set(float red, float green ,float blue, float alpha);
  /// Retrieve the component colours
  void get(float&,float&,float&,float&)const;
  /// Retrieve the component colours
  void getUB3(unsigned char* c)const;
  /// Set the painting method
  void paint(GLColor::PaintMethod pm);
private:
  /// The individual components
  float m_rgba[4];
};

#endif /*GLCOLOR_H_*/


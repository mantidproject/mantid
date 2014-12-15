#ifndef GLCOLOR_H_
#define GLCOLOR_H_

#include <iostream>

/**
  \class  GLColor
  \brief  class handling OpenGL color for objects
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  GLColor class handles the OpenGL color for an object based on the type of the 
  rendering selected. eg. MATERIAL by specifying color as glMaterial rather than
  glColor.

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class GLColor
{
public:
 
  /// Default Constructor
  GLColor(float red = 0, float green = 0,float blue = 0, float alpha = 1.0f);
  GLColor(int r, int g, int b);
  /// Destructor
  virtual ~GLColor();  

  /// Set all four values atomically
  void set(float red, float green ,float blue, float alpha);
  /// Retrieve the component colours
  void get(float&,float&,float&,float&)const;
  void get(unsigned char& r,unsigned char& g,unsigned char& b)const;
  /// Retrieve the component colours
  void getUB3(unsigned char* c)const;
  /// Set the painting method
  void paint()const;
  int red()const{return int(m_rgba[0]);}
  int green()const{return int(m_rgba[1]);}
  int blue()const{return int(m_rgba[2]);}
  int alpha()const{return int(m_rgba[3]);}

private:
  /// The individual components
  //float m_rgba[4];
  unsigned char m_rgba[4];
};

std::ostream& operator<<(std::ostream& ostr, const GLColor& c);

#endif /*GLCOLOR_H_*/


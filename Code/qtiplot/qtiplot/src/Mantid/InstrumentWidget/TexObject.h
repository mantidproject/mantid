#ifndef TEXOBJECT_H_
#define TEXOBJECT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "boost/shared_ptr.hpp"
#include "MantidObject.h"
#include "GLColor.h"
#include <vector>
#include <string>


namespace Mantid{
  namespace Geometry{
    class Object;
    class IComponent;
  }
}

/*!
\class TexObject
\brief Class for an object with a texture
\version 1.0
\date 05 Oct 2010
\author Roman Tolchenov, Tessella plc

This class shows a bank of detectors as a single object. Individual detectors are shown by different colours 
on the texture. The shape of a TexObject can be either a cylinder or a cuboid and the detectors must lie on
a straight line. This class handles the texture creation and rendering. The derived classes TexCylinder and 
TexCuboid handle the shape drawing.

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class TexObject: public MantidObject
{
public:
  TexObject(const boost::shared_ptr<const Mantid::Geometry::IComponent> obj,bool withDisplayList=true); ///< Default Constructor
  ~TexObject();								   ///< Destructor
  virtual std::string type()const {return "TexObject";} ///< Type of the GL object
  void define();  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  void setDetectorColor(int i,GLColor c); ///< set colour to a detector
  void generateTexture();
  /// Swap between drawing counts and drawing detector code colours
  void swap();
protected:
  unsigned int m_id;     ///< OpenGL texture id
  int m_n;               ///< texture size in one dimension, the other dimension is 1
  unsigned char* m_data; ///< texture colour data
  unsigned char* m_pick_data; ///< texture with detector code colours
  //std::vector<int> m_index_to_detID_map;
};

#endif /*TEXOBJECT_H_*/

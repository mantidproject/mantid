#ifndef MANTIDPLOT_GLOBJECT_H_
#define MANTIDPLOT_GLOBJECT_H_

#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {
/**
\class  GLObject
\brief  Interface for OpenGL object stored in a display list
\author Chapon Laurent & Srikanth Nagella
\date   August 2008
\version 1.0

Concrete GLObject need to overload the "define" function giving OpenGL commands
for representing
the object. The device displaying OpenGL should call the initialization of
OpenGL before any
GLObject is created otherwise glGenLists return systematically 0.

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class GLObject {
public:
  /// Type of the GL object
  static const std::string type() { return "GLObject"; }
  /// Construct a GLObject in direct rendering mode (withDisplayList=false)
  /// or using a display list (withDisplayList=true).
  /// @param withDisplayList: rendering mode
  /// @param name: name of the object
  GLObject(bool withDisplayList, const std::string &name = "");
  /// Destructor
  virtual ~GLObject();
  /// Draw the object in direct mode or using glCallList
  void draw() const;
  /// Define the drawing here.
  virtual void define() const;
  /// Don't know about this
  virtual void init() const;
  /// Set the name of the GLObject
  void setName(const std::string &name);
  /// Get the name of the GLObject
  std::string getName() const;
  /// Re-construct the opengl scene
  void construct() const;
  virtual void update() const { mChanged = true; }

protected:
  /// Name
  std::string mName;
  mutable GLuint mDisplayListId; ///< OpengGL Display list id
  mutable bool mChanged;         ///< Flag holding the change in the object
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDPLOT_GLOBJECT_H*/

// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

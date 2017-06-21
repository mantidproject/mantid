/**________________________________________________
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
#include "MantidKernel/V3D.h"
#include "GLObject.h"
#include "GLColor.h"
#include <boost/shared_ptr.hpp>

#include <ostream>

#include <QObject>
#include <QList>
#include <QRgb>

namespace Mantid {
namespace Geometry {
class IDetector;
}
}

namespace MantidQt {
namespace MantidWidgets {
class GLActorVisitor;
class GLActorConstVisitor;

/**
\class  GLActor
\brief  An actor class that holds geometry objects with its position.
\author Chapon Laurent & Srikanth Nagella
\date   August 2008
\version 1.0

Base class for all objects in a 3D Scene. Methods are provided to position and
rotate the objects.
The objects can also be set as active or not. Actors maintain safe pointer to a
GLObject.

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
enum class GLActorVisiblity : char { VISIBLE, HIDDEN, ALWAYS_HIDDEN };

class GLActor : public QObject {
public:
  /// Rules for visitor propagation. If vistor's visit(...) method returns true
  /// the propagation can be continued (VisitAll) or abandoned (Finish)
  enum VisitorAcceptRule { VisitAll, Finish };
  GLActor() : m_visible(GLActorVisiblity::VISIBLE) {}
  ///< Virtual destructor
  ~GLActor() override;
  /// Toggle the visibility of the actor.
  virtual void setVisibility(bool on);
  /// Toggle the visibility of the child actors (if exist).
  virtual void setChildVisibility(bool on) { setVisibility(on); }
  /// Sets the current component to always hide
  void setAlwaysHidden() { m_visible = GLActorVisiblity::ALWAYS_HIDDEN; }
  /// Check if any child is visible
  virtual bool hasChildVisible() const { return true; }
  /// Get the visibility status.
  bool isVisible() const { return m_visible == GLActorVisiblity::VISIBLE; }
  /// Draw the actor in 3D.
  virtual void draw(bool picking = false) const = 0;
  /// Get the 3D bounding box of the actor
  virtual void getBoundingBox(Mantid::Kernel::V3D &minBound,
                              Mantid::Kernel::V3D &maxBound) const = 0;
  /// Accept a visitor
  virtual bool accept(GLActorVisitor &visitor,
                      VisitorAcceptRule rule = VisitAll);
  /// Accept a const visitor
  virtual bool accept(GLActorConstVisitor &visitor,
                      VisitorAcceptRule rule = VisitAll) const;
  /// Convert a "pick ID" to a colour to put into the pick image.
  static GLColor makePickColor(size_t pickID);
  /// Decode a pick colour and return corresponding "pick ID"
  static size_t decodePickColor(const QRgb &c);
  /// Decode a pick colour and return corresponding "pick ID"
  static size_t decodePickColor(unsigned char r, unsigned char g,
                                unsigned char b);
  /// Get colour of a component which doesn't have any counts associated with
  /// it.
  static GLColor defaultDetectorColor();

protected:
  GLActorVisiblity m_visible; ///< Flag whether the actor is visible or not
};

} // MantidWidgets
} // MantidQt

#endif /*GLACTOR_H_*/

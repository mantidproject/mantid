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

#include <QList>
//#include <QObject>

class UnwrappedCylinder;
class UnwrappedDetectorCyl;

namespace Mantid
{
  namespace Geometry
  {
    class IDetector;
  }
}

/**
  \class  GLActor
  \brief  An actor class that holds geometry objects with its position.
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  Base class for all objects in a 3D Scene. Methods are provided to position and rotate the objects.
  The objects can also be set as active or not. Actors maintain safe pointer to a GLObject.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  virtual ~GLActor();             ///< Virtual destructor
  void setVisibility(bool);
  bool getVisibility();
  virtual void draw(bool picking = false)const = 0;
  virtual void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const = 0;
  static GLColor makePickColor(size_t pickID);
  static size_t decodePickColor(const GLColor& c);
  static size_t decodePickColor(unsigned char r,unsigned char g,unsigned char b);
  static GLColor defaultDetectorColor();
protected:
  bool  mVisible;					 ///< Flag whether the actor is visible or not
};

#endif /*GLACTOR_H_*/

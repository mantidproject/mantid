#ifndef GLACTORCOLLECTION_H_
#define GLACTORCOLLECTION_H_
#include "GLActor.h"

#include "MantidKernel/V3D.h"

#include <vector>

/**
  \class  GLActorCollection
  \brief  An actor class collection
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0


  GLActorCollection has the list of GLActor.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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


class GLActorCollection: public GLActor
{
public:
  GLActorCollection(); ///< Default Constructor
  virtual ~GLActorCollection(); ///< Destructor
  void setChildVisibility(bool);
  bool hasChildVisible() const;
  void draw(bool picking = false)const;
  void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;
  bool accept(GLActorVisitor& visitor, VisitorAcceptRule rule = VisitAll);
  bool accept(GLActorConstVisitor& visitor, VisitorAcceptRule rule = VisitAll) const;

  void addActor(GLActor*);
  void removeActor(GLActor*);
  int  getNumberOfActors();
  GLActor* getActor(int index);
  void invalidateDisplayList()const;
private:
  void drawGL(bool picking = false)const;
  mutable std::vector<GLActor*> mActorsList;    ///< Vector of GLActors for fast access.
  Mantid::Kernel::V3D m_minBound;
  Mantid::Kernel::V3D m_maxBound;
  mutable GLuint m_displayListId[2];
  mutable bool m_useDisplayList[2];
};


#endif /*GLACTORCOLLECTION_H_*/

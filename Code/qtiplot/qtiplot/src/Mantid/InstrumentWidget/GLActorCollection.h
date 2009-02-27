#ifndef GLACTORCOLLECTION_H_
#define GLACTORCOLLECTION_H_
#include "GLActor.h"
#include "GLObject.h"
#include <vector>
#include "MantidGeometry/V3D.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif



/*!
  \class  GLActorCollection
  \brief  An actor class collection
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0


  GLActorCollection has the list of GLActor.

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


class GLActorCollection: public GLObject
{
public:
	#ifndef HAS_UNORDERED_MAP_H
	typedef std::map<int,GLActor*> Actormap;
	#else
	typedef std::tr1::unordered_map<int,GLActor*> Actormap;
	#endif
	GLActorCollection(); ///< Default Constructor
	virtual ~GLActorCollection(); ///< Destructor
    void addActor(GLActor*);
    void removeActor(GLActor*);
	int  getNumberOfActors();
	GLActor* getActor(int index);
	void refresh();
    void define();
	void defineBoundingBox();
    GLActor* findColorID(unsigned char[3]);
    void drawColorID();
	void getBoundingBox(Mantid::Geometry::V3D& minPoint,Mantid::Geometry::V3D& maxPoint);
private:
    Actormap _actors;                  ///< Map of GLActors where the key is the hashed rgb color
    std::vector<GLActor*> _actorsV;    ///< Vector of GLActors for fast access.
	Mantid::Geometry::V3D _bbmin,_bmax;       ///< Bounding box min and max points
	void calculateBoundingBox();
	unsigned char referenceColorID[3];
};


#endif /*GLACTORCOLLECTION_H_*/

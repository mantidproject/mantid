#ifdef WIN32
#include <windows.h>
#endif
#include "GLActorCollection.h"
#include <iostream>
#include <functional>
#include <algorithm> 
#include <float.h>

GLActorCollection::GLActorCollection()
{
	_bbmin=Mantid::Geometry::V3D(DBL_MAX,DBL_MAX,DBL_MAX);
	_bbmax=Mantid::Geometry::V3D(-DBL_MAX,-DBL_MAX,-DBL_MAX);
}

GLActorCollection::~GLActorCollection()
{
	for(std::vector<GLActor*>::iterator i=_actors.begin();i!=_actors.end();i++)
		delete (*i);
	_actors.clear();
}

/**
 * This method does the drawing by calling the list of actors to draw themselfs
 */
void GLActorCollection::define()
{
    for_each(_actors.begin(),_actors.end(),std::mem_fun(&GLActor::draw));
}

/**
 * This method call the bounding box method of the all actors
 */
void GLActorCollection::drawBoundingBox()
{
    for_each(_actors.begin(),_actors.end(),std::mem_fun(&GLActor::drawBoundingBox));
}

/**
 * This method addes a new actor to the collection.
 * @param a input actor to be added to the list
 */
void GLActorCollection::addActor(GLActor* a)
{
    _actors.push_back(a);
	Mantid::Geometry::V3D tmin,tmax;
    a->getBoundingBox(tmin,tmax);
    if(_bbmin[0]>tmin[0]) _bbmin[0]=tmin[0];
	if(_bbmin[1]>tmin[1]) _bbmin[1]=tmin[1];
	if(_bbmin[2]>tmin[2]) _bbmin[2]=tmin[2];
	if(_bbmax[0]<tmax[0]) _bbmax[0]=tmax[0];
	if(_bbmax[1]<tmax[1]) _bbmax[1]=tmax[1];
	if(_bbmax[2]<tmax[2]) _bbmax[2]=tmax[2];
}

/**
 * Remove the input actor from the collection
 * @param a input actor to be removed from the list
 */
void GLActorCollection::removeActor(GLActor* a)
{
    if (!a) return;
	Mantid::Geometry::V3D tmin,tmax;
	a->getBoundingBox(tmin,tmax);
	bool bRecalculate=false;
	if(_bbmin[0]==tmin[0]||_bbmin[1]==tmin[1]||_bbmin[2]==tmin[2]||_bbmax[0]==tmax[0]||_bbmax[1]==tmax[1]||_bbmax[2]==tmax[2]) bRecalculate=true;
    std::vector<GLActor*>::iterator i;
    i=find(_actors.begin(),_actors.end(),a);
    if (i!=_actors.end()) _actors.erase(i);
	if(bRecalculate) calculateBoundingBox();
}

/**
 * This method returns the number of actors in the collection
 * @return integer value of number of actors in collection
 */
int  GLActorCollection::getNumberOfActors()
{
	return _actors.size();
}

/**
 * This method returns the actor at the given index
 * @param index is the index in actor collection to be returned
 * @return a pointer to the actor at a given index 
 */
GLActor* GLActorCollection::getActor(int index)
{
	if(index<0||index>_actors.size())return NULL;
	return _actors.at(index);
}

/**
 * This method is to find the matching input color id in the collection of actors color and mark the matched actor as picked.
 * @param color input color
 */
GLActor* GLActorCollection::findColorID(unsigned char color[3])
{
    //sort(_actors.begin(),_actors.end());
  //  std::vector<GLActor*>::iterator i;
  //  for (i=_actors.begin();i!=_actors.end();i++)
  //  {
		//if ((*i)->isColorID(color)) 
		//{
  //        (*i)->markPicked();
		//  break;
		//}
  //  }
	GLActor* picked=NULL;
	std::cout<<"Number of actors "<<_actors.size()<<std::endl;
	for(int i=0;i<_actors.size();i++)
	{
		if(_actors[i]->isColorID(color))
		{
			_actors[i]->markPicked();
			picked=_actors[i];
		}
		else
		{
			_actors[i]->markUnPicked();
		}
	}
  //  define();
  //  if (i!=_actors.end()) return *i;
  //  else return 0;
	return picked;
}

/**
 * This method draws the collection of actors using the color id method of actors
 */
void GLActorCollection::drawColorID()
{
    for_each(_actors.begin(),_actors.end(),std::mem_fun(&GLActor::drawIDColor));
}

/**
 * This method returns the points of bounding box that bounds complete actor collection
 * @param minPoint output min point of the bounding box
 * @param maxPoint output max point of the bounding box
 */
void GLActorCollection::getBoundingBox(Mantid::Geometry::V3D& minPoint,Mantid::Geometry::V3D& maxPoint)
{
	if(_actors.size()==0)
	{
		minPoint=Mantid::Geometry::V3D(0,0,0);
		maxPoint=Mantid::Geometry::V3D(0,0,0);
	}else{
		minPoint=_bbmin;
		maxPoint=_bbmax;
	}
}

/**
 * This method redraws the entire scene because of change in the color
 */
void GLActorCollection::refresh()
{
	this->_changed=true;
}

/**
 * Recalculates the bounding box values. invoked when a actor is removed.
 */
void GLActorCollection::calculateBoundingBox()
{
    std::vector<GLActor*>::iterator i;
	_bbmin=Mantid::Geometry::V3D(DBL_MAX,DBL_MAX,DBL_MAX);
	_bbmax=Mantid::Geometry::V3D(-DBL_MAX,-DBL_MAX,-DBL_MAX);
    for (i=_actors.begin();i!=_actors.end();i++)
    {
		Mantid::Geometry::V3D tmin,tmax;
        (*i)->getBoundingBox(tmin,tmax);
        if(_bbmin[0]>tmin[0]) _bbmin[0]=tmin[0];
		if(_bbmin[1]>tmin[1]) _bbmin[1]=tmin[1];
		if(_bbmin[2]>tmin[2]) _bbmin[2]=tmin[2];
		if(_bbmin[0]<tmax[0]) _bbmin[0]=tmax[0];
		if(_bbmin[1]<tmax[1]) _bbmin[1]=tmax[1];
		if(_bbmin[2]<tmax[2]) _bbmin[2]=tmax[2];
    }
}

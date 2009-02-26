#ifdef WIN32
#include <windows.h>
#endif
#include "GLActorCollection.h"
#include <iostream>
#include <functional>
#include <algorithm>
#include <float.h>

static int hash(unsigned char r, unsigned char g, unsigned char b)
{
	 return r*65536+g*256+b;
}

GLActorCollection::GLActorCollection():GLObject(false)
{
	referenceColorID[0]=0;referenceColorID[1]=0;referenceColorID[2]=0;
	_bbmin=Mantid::Geometry::V3D(DBL_MAX,DBL_MAX,DBL_MAX);
	_bbmax=Mantid::Geometry::V3D(-DBL_MAX,-DBL_MAX,-DBL_MAX);
}

GLActorCollection::~GLActorCollection()
{
	_actors.clear();
	for(std::vector<GLActor*>::iterator i=_actorsV.begin();i!=_actorsV.end();i++)
			delete (*i);
	_actorsV.clear();
}

/**
 * This method does the drawing by calling the list of actors to draw themselfs
 */
void GLActorCollection::define()
{
    for_each(_actorsV.begin(),_actorsV.end(),std::mem_fun(&GLActor::draw));
}

/**
 * This method call the bounding box method of the all actors
 */
void GLActorCollection::drawBoundingBox()
{
    for_each(_actorsV.begin(),_actorsV.end(),std::mem_fun(&GLActor::drawBoundingBox));
}

/**
 * This method addes a new actor to the collection.
 * @param a input actor to be added to the list
 */
void GLActorCollection::addActor(GLActor* a)
{
	if (!a)
		return;
	a->setColorID(referenceColorID[0],referenceColorID[1],referenceColorID[2]);
	int key=hash(referenceColorID[0],referenceColorID[1],referenceColorID[2]);
    _actors[key]=a;
    _actorsV.push_back(a);

    referenceColorID[0]++;
    if (referenceColorID[0]>254)
	{
	   referenceColorID[0]=0;
	   referenceColorID[1]++;
	   if (referenceColorID[1]>254)
	   {
		 referenceColorID[1]=0;
		 referenceColorID[2]++;
	   }
	}
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
//    if (!a) return;
//	Mantid::Geometry::V3D tmin,tmax;
//	a->getBoundingBox(tmin,tmax);
//	bool bRecalculate=false;
//	if(_bbmin[0]==tmin[0]||_bbmin[1]==tmin[1]||_bbmin[2]==tmin[2]||_bbmax[0]==tmax[0]||_bbmax[1]==tmax[1]||_bbmax[2]==tmax[2]) bRecalculate=true;
//    std::vector<GLActor*>::iterator i;
//    i=find(_actors.begin(),_actors.end(),a);
//    if (i!=_actors.end()) _actors.erase(i);
//	if(bRecalculate) calculateBoundingBox();
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
	return _actorsV.at(index);
}

/**
 * This method is to find the matching input color id in the collection of actors color and mark the matched actor as picked.
 * @param color input color
 */
GLActor* GLActorCollection::findColorID(unsigned char color[3])
{
	GLActor* picked=0;
	int key=hash(color[0],color[1],color[2]);
	Actormap::const_iterator it=_actors.find(key);
	if (it!=_actors.end())
	{
		picked=(*it).second;
		picked->markPicked();
	}
	return picked;
}

/**
 * This method draws the collection of actors using the color id method of actors
 */
void GLActorCollection::drawColorID()
{
    for_each(_actorsV.begin(),_actorsV.end(),std::mem_fun(&GLActor::drawIDColor));
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
		minPoint=Mantid::Geometry::V3D(-1,-1,-1);
		maxPoint=Mantid::Geometry::V3D(1,1,1);
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
    for (i=_actorsV.begin();i!=_actorsV.end();i++)
    {
		Mantid::Geometry::V3D tmin,tmax;
        (*i)->getBoundingBox(tmin,tmax);
        if(_bbmin[0]>tmin[0]) _bbmin[0]=tmin[0];
		if(_bbmin[1]>tmin[1]) _bbmin[1]=tmin[1];
		if(_bbmin[2]>tmin[2]) _bbmin[2]=tmin[2];
		if(_bbmax[0]<tmax[0]) _bbmax[0]=tmax[0];
		if(_bbmax[1]<tmax[1]) _bbmax[1]=tmax[1];
		if(_bbmax[2]<tmax[2]) _bbmax[2]=tmax[2];
    }
}

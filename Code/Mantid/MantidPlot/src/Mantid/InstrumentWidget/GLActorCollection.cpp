#include "GLActorCollection.h"
#include <stdexcept>
#include <iostream>
#include <functional>
#include <algorithm>
#include <float.h>

//static int hash(unsigned char r, unsigned char g, unsigned char b)
//{
//	 return r*65536+g*256+b;
//}

GLActorCollection::GLActorCollection():GLObject(true)
{
	referenceColorID[0] = 0;
	referenceColorID[1] = 0;
	referenceColorID[2] = 1;
}

GLActorCollection::~GLActorCollection()
{
	for(std::vector<GLActor*>::iterator i=mActorsList.begin();i!=mActorsList.end();i++)
	{
	  delete (*i);
	}
	mActorsList.clear();
}

/**
 * This method does the drawing by calling the list of actors to draw themselfs
 */
void GLActorCollection::define()
{
    for_each(mActorsList.begin(),mActorsList.end(),std::mem_fun(&GLActor::draw));
}


/**
 * This method addes a new actor to the collection.
 * @param a :: input actor to be added to the list
 */
void GLActorCollection::addActor(GLActor* a)
{
        if( !a )
	{
	  return;
	}
	mActorsList.push_back(a);
	int rgb = referenceColorID[0]*65536 + referenceColorID[1]*256 + referenceColorID[2];
	int noOfColors = a->setStartingReferenceColor(rgb);
	rgb += noOfColors;
	referenceColorID[0] = rgb / 65536;
	referenceColorID[1] = (rgb % 65536) / 256;
	referenceColorID[2] = (rgb % 65536) % 256;
}

/**
 * Remove the input actor from the collection
 * @param a :: input actor to be removed from the list
 */
void GLActorCollection::removeActor(GLActor*)
{
	throw std::runtime_error("Removing actor not implemented");
}

/**
 * This method returns the number of actors in the collection
 * @return integer value of number of actors in collection
 */
int  GLActorCollection::getNumberOfActors()
{
	return mActorsList.size();
}

/**
 * This method returns the actor at the given index
 * @param index :: is the index in actor collection to be returned
 * @return a pointer to the actor at a given index
 */
GLActor* GLActorCollection::getActor(int index)
{
	if(index<0||index> static_cast<int>(mActorsList.size()) )return NULL;
	return mActorsList.at(index);
}

/**
 * This method is to find the matching input color id in the collection of actors color and mark the matched actor as picked.
 * @param color :: input color
 */
GLActor* GLActorCollection::findColorID(unsigned char color[3])
{
  (void) color; //avoid compiler warning

	GLActor* picked=0;
	throw std::runtime_error("Find colorid not implemented");
	//int key=hash(color[0],color[1],color[2]);
	//Actormap::const_iterator it=_actors.find(key);
	//if (it!=_actors.end())
	//{
	//	picked=(*it).second;
	//	picked->markPicked();
	//}
	return picked;
}

/**
 * This method redraws the entire scene because of change in the color
 */
void GLActorCollection::refresh()
{
	this->mChanged=true;
}

/**
 * This method calls all the initialisation of the Actors in the collection
 */
void GLActorCollection::init()
{
    for_each(mActorsList.begin(),mActorsList.end(),std::mem_fun(&GLActor::init));
}

//void GLActorCollection::addToUnwrappedList(UnwrappedCylinder& cylinder, QList<UnwrappedDetectorCyl>& list)
//{
//  std::vector<GLActor*>::iterator it = mActorsList.begin();
//  for(;it != mActorsList.end();++it)
//  {
//    (**it).addToUnwrappedList(cylinder,list);
//  }
//}

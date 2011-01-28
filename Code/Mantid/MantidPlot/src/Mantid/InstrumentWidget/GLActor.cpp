#include "GLActor.h"


boost::shared_ptr<GLColor> redColor(new GLColor(1.0,0.5,0.0,1.0));

GLActor::GLActor(bool withDisplayList):
    GLObject(withDisplayList), mPicked(false), mVisible(true)
{
	mColor=redColor;
}

GLActor::~GLActor()
{
}


/**
 * This method marks the Actor as picked
 */
void GLActor::markPicked()
{
    mPicked=true;
}

/**
 * This method unmarks/clears the Actor pick. set the status as unpicked
 */
void GLActor::markUnPicked()
{
    mPicked=false;
}

/**
 * This method sets the color for the actor
 * @param color :: input color for the actor
 */
void GLActor::setColor(boost::shared_ptr<GLColor> color)
{
    mColor=color;
}

/**
 * This method sets the visiblity of the actor
 * @param visible :: input visibility ( true for visible and false for hidden
 */
void GLActor::setVisibility(bool visible)
{
	mVisible=visible;
}

/**
 * This method gets the visibility of the actor
 * @return true for actor visible and false for hidden
 */
bool GLActor::getVisibility()
{
	return mVisible;
}

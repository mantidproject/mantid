#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "ObjComponentActor.h"
#include "InstrumentActor.h"

using namespace Mantid::Geometry;

/**
 * Constructor
 * @param ins             :: Instrument
 * @param withDisplayList :: true use display list, false use immediate rendering
 */
InstrumentActor::InstrumentActor(boost::shared_ptr<Mantid::Geometry::IInstrument> ins, bool withDisplayList):CompAssemblyActor(withDisplayList)
{
	mInstrument=ins;
	mId=mInstrument->getComponentID();
	mObjects = boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> >(new std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*>());
	initChilds(withDisplayList);
}

/**
 * Destructor
 */
InstrumentActor::~InstrumentActor()
{
	for(std::map<const boost::shared_ptr<const Object>,MantidObject*>::iterator iObjs=mObjects->begin();iObjs!=mObjects->end();iObjs++)
	{
		delete (*iObjs).second;
	}
	mObjects->clear();
}

/**
 * This method returns the detector id's in the instrument
 * @param idList :: output a list of the detector id's in the instrument
 */
void InstrumentActor::getDetectorIDList(std::vector<int>& idList)
{
	this->appendObjCompID(idList);
}

/**
 * This method sets the colors for the detectors, the list order should be the same as the getDetectorIDList output
 * @param list: list of detector colors, this should match the same order the detector id list in the getDetectorIDList
 */
void InstrumentActor::setDetectorColors(std::vector<boost::shared_ptr<GLColor> >& list)
{
	std::vector<boost::shared_ptr<GLColor> >::iterator listItr=list.begin();
	setInternalDetectorColors(listItr);
}

/**
 * Redraws the instrument
 */
void InstrumentActor::refresh()
{
	this->redraw();
	this->draw();
}

/** 
 * This method returns the detector id corresponding to the input reference color 
 * @param rgb:
 * @return the detector id correspondign to rgb.
 */
int InstrumentActor::getDetectorIDFromColor(int rgb)
{
	if(rgb==0) return -1;
	return findDetectorIDUsingColor(rgb);
}

/**
 * This method sets the instrument to render in low resolution. Useful rendering in interaction mode
 */
void InstrumentActor::setObjectResolutionToLow()
{
	for(std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*>::iterator itr=mObjects->begin();itr!=mObjects->end();itr++)
	{
		(itr->second)->setResolutionToLow();
	}
}

/**
 * This method sets the instrument to render in high resolution.
 */
void InstrumentActor::setObjectResolutionToHigh()
{
	for(std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*>::iterator itr=mObjects->begin();itr!=mObjects->end();itr++)
	{
		(itr->second)->setResolutionToHigh();
	}
}

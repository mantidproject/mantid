#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ParObjCompAssembly.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "ObjCompAssemblyActor.h"
#include "ObjComponentActor.h"
#include "TexObject.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;

ObjCompAssemblyActor::ObjCompAssemblyActor(boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> >& objs,
    Mantid::Geometry::ComponentID id, 
    boost::shared_ptr<Mantid::Geometry::IInstrument> ins,
    bool withDisplayList):
  ICompAssemblyActor(objs,id,ins,withDisplayList)
{
  boost::shared_ptr<IComponent> ic = ins->getComponentByID(id);
  boost::shared_ptr<ObjCompAssembly> oca = boost::dynamic_pointer_cast<ObjCompAssembly>(ic);
  if (oca)
  {
    m_ObjAss = oca;
  }
  else
  {
    boost::shared_ptr<ParObjCompAssembly> poca = boost::dynamic_pointer_cast<ParObjCompAssembly>(ic);
    if (!poca)
    {
      throw Mantid::Kernel::Exception::InstrumentDefinitionError("Expected ParObjCompAssembly, found "+ic->type());
    }
    m_ObjAss.reset(dynamic_cast<const ObjCompAssembly*>(poca->base()),NoDeleting());
  }
  if (!m_ObjAss)
  {
    throw Mantid::Kernel::Exception::InstrumentDefinitionError("Expected ObjCompAssembly, found "+ic->type());
  }
  setName(m_ObjAss->getName());
  m_tex.reset(new TexObject(m_ObjAss,withDisplayList));
  initChilds(withDisplayList);
}

/**
* Destructor which removes the actors created by this object
*/
ObjCompAssemblyActor::~ObjCompAssemblyActor()
{
}

/**
* This function is concrete implementation that renders the Child ObjComponents and Child CompAssembly's
*/
void ObjCompAssemblyActor::define()
{
  //Only draw the CompAssembly Children only if they are visible
  if(mVisible)
  {
    glPushMatrix();
    // Translation first
    V3D pos = m_ObjAss->getPos();
    if (!(pos.nullVector()))
    {
            glTranslated(pos[0],pos[1],pos[2]);
    }
    //Rotation
    Quat rot = m_ObjAss->getRotation();
    if (!(rot.isNull()))
    {
            double deg,ax0,ax1,ax2;
            rot.getAngleAxis(deg,ax0,ax1,ax2);
            glRotated(deg,ax0,ax1,ax2);
    }
    //Scale
    V3D scaleFactor = m_ObjAss->getScaleFactor();
    if (!(scaleFactor==V3D(1,1,1)))
    {
            glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
    }

    //std::cout << "ObjCompAssemblyActor::define() called with pos=" << pos << " and rot=" << rot << " and scale=" << scaleFactor << ".\n";

    m_tex->define();

    glPopMatrix();
  }
}

/**
* This method draws the children with the ColorID rather than the children actual color. used in picking the component
*/
void ObjCompAssemblyActor::drawUsingColorID()
{
  //Check whether this assembly and its child components can be drawn
  if(mVisible)
  {
    m_tex->swap(); // swap to pick texture
    //Iterate throught the children with the starting reference color mColorStartID and incrementing
    int rgb=mColorStartID;
    int r,g,b;
    for(int i = 0;i < getNumberOfDetectors();++i)
    {
      r=(rgb/65536);
      g=((rgb%65536)/256);
      b=((rgb%65536)%256);
      GLColor c(r/255.0,g/255.0,b/255.0);
      m_tex->setDetectorColor(i,c);
      rgb++;
    }
    m_tex->generateTexture();
    define();
    m_tex->swap(); // swap to show the data
    m_tex->generateTexture();
  }
}

/**
* Initialises the CompAssembly Children and creates actors for each children
* @param withDisplayList :: the new actors for the children with same display list attribute as parent
*/
void ObjCompAssemblyActor::initChilds(bool withDisplayList)
{
  mNumberOfDetectors = m_ObjAss->nelements();
  for(int i=0;i<getNumberOfDetectors();++i)
  {
    mObjCompIDs.push_back(m_ObjAss->getChild(i)->getComponentID());
  }

  double xmin,ymin,zmin,xmax,ymax,zmax;
  xmin=ymin=zmin=-1000;
  xmax=ymax=zmax=1000;
  m_ObjAss->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
  minBoundBox[0]=xmin;minBoundBox[1]=ymin;minBoundBox[2]=zmin;
  maxBoundBox[0]=xmax;maxBoundBox[1]=ymax;maxBoundBox[2]=zmax;
}

/**
* This method checks the list of Objects in mObjects for obj, if found returns the MantidObject. if the obj is not found
* then creates a new MantidObject for obj and adds to the list of mObjects and returns the newly created MantidObject.
* @param obj             :: Object input for which MantidObject needed
* @param withDisplayList :: whether the new MantidObject if needed uses the display list attribute
* @return  the MantidObject corresponding to the obj.
*/
MantidObject*	ObjCompAssemblyActor::getMantidObject(const boost::shared_ptr<const Mantid::Geometry::Object> obj,bool withDisplayList)
{
  if (!obj)
  {
    throw std::runtime_error("An instrument component does not have a shape.");
  }
  std::map<const boost::shared_ptr<const Object>,MantidObject*>::iterator iObj=mObjects->find(obj);
  if(iObj==mObjects->end()) //create an new Mantid Object
  {
    MantidObject* retObj=new MantidObject(obj,withDisplayList);
    retObj->draw();
    mObjects->insert(mObjects->begin(),std::pair<const boost::shared_ptr<const Object>,MantidObject*>(obj,retObj));
    return retObj;
  }
  return (*iObj).second;
}

/**
* Set the starting color reference for CompAssembly
* @param rgb :: input color id
* @return  the number of color ids that are used.
*/
int ObjCompAssemblyActor::setStartingReferenceColor(int rgb)
{
  mColorStartID=rgb;
  return getNumberOfDetectors();
}

/**
* Concrete implementation of init method of GLObject. this method draws the children
*/
void ObjCompAssemblyActor::init()
{
}

/**
* This method adds the detector ids of the children of CompAssembly to the input idList.
* @param idList :: output list of detector ids for child detectors
*/
void ObjCompAssemblyActor::appendObjCompID(std::vector<int>& idList)
{

  for(int i=0;i<getNumberOfDetectors();++i)
  {
    idList.push_back(boost::dynamic_pointer_cast<IDetector>(m_ObjAss->getChild(i))->getID());
  }
}

/**
* The colors are set using the iterator of the color list.
* @param list :: Color list iterator
* @return the number of detectors
*/
int ObjCompAssemblyActor::setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator& list)
{
  for(int i=0;i<getNumberOfDetectors();++i)
  {
    m_tex->setDetectorColor(i,**list);
    list++;
  }
  m_tex->generateTexture();
  return getNumberOfDetectors();
}

/**
* This method redraws the CompAssembly children compassembly actors redraw. this method is used to redraw all the children
*/
void ObjCompAssemblyActor::redraw()
{
  mChanged=true;
  construct();
}

/**
* This method searches the child actors for the input rgb color and returns the detector id corresponding to the rgb color. if the
* detector is not found then returns -1.
* @param  rgb :: input color id  ? I am not sure what it is ? It looks like an index of a child component + 1
* @return the detector id of the input rgb color.if detector id not found then returns -1
*/
int ObjCompAssemblyActor::findDetectorIDUsingColor(int rgb)
{
  int i = rgb - 1;
  if (i >= 0 && i < getNumberOfDetectors())
  {
    return boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>(m_ObjAss->getChild(i))->getID();
  }
  return -1;
}


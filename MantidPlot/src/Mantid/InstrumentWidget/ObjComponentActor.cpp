#include "ObjComponentActor.h"
#include "InstrumentActor.h"
#include "OpenGLError.h"

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"

using namespace Mantid;
using namespace Geometry;

ObjComponentActor::ObjComponentActor(const InstrumentActor& instrActor,Mantid::Geometry::ComponentID compID)
  : ComponentActor(instrActor,compID)
{
  // set the displayed colour
  setColors();
  // register the component with InstrumentActor and set the pick colour
  IDetector_const_sptr det = getDetector();
  if (det)
  {
    size_t pickID = instrActor.pushBackDetid(det->getID());
    m_pickColor = makePickColor(pickID);
  }
  else
  {
    instrActor.pushBackNonDetid(this,compID);
  }
}

ObjComponentActor::~ObjComponentActor()
{
}

//-------------------------------------------------------------------------------------------------
/**
 * Concrete implementation of rendering ObjComponent.
 */
void ObjComponentActor::draw(bool picking)const
{
  OpenGLError::check("ObjComponentActor::draw(0)");
  glPushMatrix();
  if (picking)
  {
    m_pickColor.paint();
  }
  else
  {
    m_dataColor.paint();
  }
  getObjComponent()->draw();
  glPopMatrix();
  OpenGLError::check("ObjComponentActor::draw()");
}

/**
 * Set displayed component colour. If it's a detector the colour maps to the integrated counts in it.
 */
void ObjComponentActor::setColors()
{
  IDetector_const_sptr det = getDetector();
  if (det)
  {
    setColor(m_instrActor.getColor(det->getID()));
  }
  else
  {
    setColor(defaultDetectorColor());
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Return the bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void ObjComponentActor::getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const
{
  Mantid::Geometry::BoundingBox boundBox;
  getComponent()->getBoundingBox(boundBox);
  minBound = boundBox.minPoint();
  maxBound = boundBox.maxPoint();
}


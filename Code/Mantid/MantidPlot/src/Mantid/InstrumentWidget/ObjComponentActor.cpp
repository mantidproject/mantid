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
  setColors();
  IDetector_const_sptr det = getDetector();
  if (det)
  {
    size_t pickID = instrActor.push_back_detid(det->getID());
    m_pickColor = makePickColor(pickID);
  }
  else
  {
    m_pickColor = GLColor();
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


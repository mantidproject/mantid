#include "MantidQtMantidWidgets/InstrumentView/ObjComponentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/OpenGLError.h"

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"

using namespace Mantid;
using namespace Geometry;

namespace {
// Anonymous namespace

/**
  * Returns if the current component is finite or
  * has 'infinite' length based on all axis found
  * within the bounding box
  *
  * @param compID:: The component to check
  * @return :: True if the component has finite size else False
  */
bool isComponentFinite(const Mantid::Geometry::ComponentID &compID) {
  Geometry::BoundingBox boundedBox;
  compID->getBoundingBox(boundedBox);
  const auto width = boundedBox.width();
  const double x = width[0];
  const double y = width[1];
  const double z = width[2];

  // Currently an 'infinite' component will have length 1000
  // on one of its axis. Check all to make sure it is not greater
  // than 1000 units in length.
  const double maximumSize = 999;
  if (x > maximumSize || y > maximumSize || z > maximumSize) {
    return false;
  } else {
    return true;
  }
}
}

namespace MantidQt {
namespace MantidWidgets {

ObjComponentActor::ObjComponentActor(const InstrumentActor &instrActor,
                                     Mantid::Geometry::ComponentID compID)
    : ComponentActor(instrActor, compID) {
  // set the displayed colour
  setColors();

  if (!isComponentFinite(compID)) {
    // If the component does not have finite length we set it always
    // hidden so scale is not messed up and it is not displayed.
    setAlwaysHidden();
  }

  // register the component with InstrumentActor and set the pick colour
  IDetector_const_sptr det = getDetector();
  if (det) {
    size_t pickID = instrActor.pushBackDetid(det->getID());
    m_pickColor = makePickColor(pickID);
  } else {
    instrActor.pushBackNonDetid(this, compID);
  }
}

ObjComponentActor::~ObjComponentActor() {}

//-------------------------------------------------------------------------------------------------
/**
* Concrete implementation of rendering ObjComponent.
*/
void ObjComponentActor::draw(bool picking) const {
  OpenGLError::check("ObjComponentActor::draw(0)");
  glPushMatrix();
  if (picking) {
    m_pickColor.paint();
  } else {
    m_dataColor.paint();
  }
  getObjComponent()->draw();
  glPopMatrix();
  OpenGLError::check("ObjComponentActor::draw()");
}

/**
* Set displayed component colour. If it's a detector the colour maps to the
* integrated counts in it.
*/
void ObjComponentActor::setColors() {
  IDetector_const_sptr det = getDetector();
  if (det) {
    setColor(m_instrActor.getColor(det->getID()));
  } else {
    setColor(defaultDetectorColor());
  }
}

/**
* Return the bounding box of visible components.
* If this is not visible an empty V3D object will
* be returned.
* @param minBound :: min point of the bounding box
* @param maxBound :: max point of the bounding box
*/
void ObjComponentActor::getBoundingBox(Mantid::Kernel::V3D &minBound,
                                       Mantid::Kernel::V3D &maxBound) const {
  if (!isVisible()) {
    // If this is not visible we should not consider this component
    minBound = Kernel::V3D();
    maxBound = Kernel::V3D();
  } else {
    Mantid::Geometry::BoundingBox boundBox;
    getComponent()->getBoundingBox(boundBox);
    minBound = boundBox.minPoint();
    maxBound = boundBox.maxPoint();
  }
}

} // MantidWidgets
} // MantidQt
#include "MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/ObjComponentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/StructuredDetectorActor.h"

#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor.
*
* @param instrActor :: the instrument actor
* @param compID :: the component ID
*/
StructuredDetectorActor::StructuredDetectorActor(
    const InstrumentActor &instrActor,
    const Mantid::Geometry::ComponentID &compID)
    : ICompAssemblyActor(instrActor, compID){

  mNumberOfDetectors = 0;
  m_det = boost::dynamic_pointer_cast<const StructuredDetector>(getComponent());

  if (!m_det)
    return;

  BoundingBox compBox;
  m_det->getBoundingBox(compBox);
  mNumberOfDetectors = m_det->xPixels() * m_det->yPixels();
  this->AppendBoundingBox(compBox.minPoint(), compBox.maxPoint());

  
  for (size_t y = 0; y < m_det->yPixels(); y++) {
    for (size_t x = 0; x < m_det->xPixels(); x++) {
      // Getting the detector is slow. Get the ID directly
      detid_t id = m_det->getDetectorIDAtXY(x, y);
      size_t pickID = instrActor.pushBackDetid(id);
	  m_pickIds.push_back(pickID);
	  m_pickColors.push_back(GLActor::makePickColor(pickID));
      m_clist.push_back(instrActor.getColor(id));
    }
  }
}

/**
* Destructor which removes the actors created by this object
*/
StructuredDetectorActor::~StructuredDetectorActor() {
}

void StructuredDetectorActor::draw(bool picking) const {
  glPushMatrix();
  // Translation first
  V3D pos = m_det->getPos();
  if (!(pos.nullVector())) {
    glTranslated(pos[0], pos[1], pos[2]);
  }
  // Rotation
  Quat rot = m_det->getRotation();
  if (!(rot.isNull())) {
    double deg, ax0, ax1, ax2;
    rot.getAngleAxis(deg, ax0, ax1, ax2);
    glRotated(deg, ax0, ax1, ax2);
  }
  // Scale
  V3D scaleFactor = m_det->getScaleFactor();
  if (!(scaleFactor == V3D(1, 1, 1))) {
    glScaled(scaleFactor[0], scaleFactor[1], scaleFactor[2]);
  }

  // StructuredDetector will use.
  std::vector<int> r, g, b;

  if (picking) {
    for (std::vector<GLColor>::const_iterator i = m_pickColors.cbegin();
         i != m_pickColors.cend(); ++i) {
      r.push_back((*i).red());
      g.push_back((*i).green());
      b.push_back((*i).blue());
    }
  } else {
    for (std::vector<GLColor>::const_iterator i = m_clist.cbegin();
         i != m_clist.cend(); ++i) {
      r.push_back((*i).red());
      g.push_back((*i).green());
      b.push_back((*i).blue());
    }
  }

  m_det->setColors(r, g, b);
  m_det->draw();

  glPopMatrix();
}

/**
* Accept a visitor. This sets the matching component's visibility to True.
* It looks if the given component is a child (pixel) of the parent rectangular
* detector, and sets the visibility of the whole panel to true if so.
*
* @param visitor :: A visitor.
* @param rule :: A rule defining visitor acceptance by assembly actors. Unused.
*
*/
bool StructuredDetectorActor::accept(GLActorVisitor &visitor,
                                     VisitorAcceptRule) {
  return visitor.visit(this);
}

bool StructuredDetectorActor::accept(GLActorConstVisitor &visitor,
                                     VisitorAcceptRule) const {
  return visitor.visit(this);
}

bool StructuredDetectorActor::isChildDetector(
    const Mantid::Geometry::ComponentID &id) const {
  // ID of the parent RectangularDetector
  Mantid::Geometry::ComponentID thisID = this->m_id;

  // Get the component object
  IComponent_const_sptr comp =
      m_instrActor.getInstrument()->getComponentByID(id);
  if (comp) {
    // Get the parent (e.g. the column)
    IComponent_const_sptr parent1 = comp->getParent();
    if (parent1) {
      if (parent1->getComponentID() == thisID) {
        return true;
      }
      // Go to grandparent
      IComponent_const_sptr parent2 = parent1->getParent();
      if (parent2) {
        if (parent2->getComponentID() == thisID) {
          return true;
        }
      } // valid grandparent
    }   // valid parent
  }     // valid component
  return false;
}

//-------------------------------------------------------------------------------------------------
/**
* Return the bounding box, from the one calculated in the cache previously.
* @param minBound :: min point of the bounding box
* @param maxBound :: max point of the bounding box
*/
void StructuredDetectorActor::getBoundingBox(
    Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound) const {
  minBound = minBoundBox;
  maxBound = maxBoundBox;
}

/**
* Append the bounding box CompAssembly bounding box
* @param minBound :: min point of the bounding box
* @param maxBound :: max point of the bounding box
*/
void StructuredDetectorActor::AppendBoundingBox(
    const Mantid::Kernel::V3D &minBound, const Mantid::Kernel::V3D &maxBound) {
  if (minBoundBox[0] > minBound[0])
    minBoundBox[0] = minBound[0];
  if (minBoundBox[1] > minBound[1])
    minBoundBox[1] = minBound[1];
  if (minBoundBox[2] > minBound[2])
    minBoundBox[2] = minBound[2];
  if (maxBoundBox[0] < maxBound[0])
    maxBoundBox[0] = maxBound[0];
  if (maxBoundBox[1] < maxBound[1])
    maxBoundBox[1] = maxBound[1];
  if (maxBoundBox[2] < maxBound[2])
    maxBoundBox[2] = maxBound[2];
}

void StructuredDetectorActor::setColors() {
 //do nothing
}

} // namespace MantidWidgets
} // namespace MantidQt
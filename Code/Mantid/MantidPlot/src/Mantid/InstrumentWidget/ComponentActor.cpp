#include "ComponentActor.h"
#include "InstrumentActor.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid;
using namespace Geometry;

ComponentActor::ComponentActor(const InstrumentActor& instrActor, const Mantid::Geometry::ComponentID& compID)
  : GLActor(),
  m_instrActor(instrActor),
	m_id(compID)
{
}

bool ComponentActor::accept(GLActorVisitor &visitor, GLActor::VisitorAcceptRule)
{
    return visitor.visit(this);
}

boost::shared_ptr<const Mantid::Geometry::IComponent> ComponentActor::getComponent() const
{
  return m_instrActor.getInstrument()->getComponentByID(m_id);
}

boost::shared_ptr<const Mantid::Geometry::IObjComponent> ComponentActor::getObjComponent() const
{
  return boost::dynamic_pointer_cast<const Mantid::Geometry::IObjComponent>(getComponent());
}

boost::shared_ptr<const Mantid::Geometry::IDetector> ComponentActor::getDetector() const
{
  return boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(getComponent());
}

boost::shared_ptr<const Mantid::Geometry::ObjCompAssembly> ComponentActor::getObjCompAssembly() const
{
  return boost::dynamic_pointer_cast<const Mantid::Geometry::ObjCompAssembly>(getComponent());
}

/**
 * An component is a non-detector if it's an ObjComponent (has a shape) and not an ObjCompAssembly
 * (a single object) and not a RectangularDetector (which is an assembly).
 */
bool ComponentActor::isNonDetector() const
{
  auto obj = getObjComponent();
  return  obj && 
          !getObjCompAssembly() && 
          !getDetector() &&
          !boost::dynamic_pointer_cast<const Mantid::Geometry::RectangularDetector>(obj);
}


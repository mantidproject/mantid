#include "ComponentActor.h"
#include "InstrumentActor.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"

using namespace Mantid;
using namespace Geometry;

ComponentActor::ComponentActor(const InstrumentActor& instrActor, const Mantid::Geometry::ComponentID& compID)
  : GLActor(),
  m_instrActor(instrActor),
	m_id(compID)
{
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


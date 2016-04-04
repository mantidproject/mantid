#include "MantidQtMantidWidgets/InstrumentView/ComponentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

using namespace Mantid;
using namespace Geometry;

namespace MantidQt
{
	namespace MantidWidgets
	{
		ComponentActor::ComponentActor(const InstrumentActor &instrActor, const Mantid::Geometry::ComponentID &compID)
			: GLActor(), m_instrActor(instrActor), m_id(compID) {}

		bool ComponentActor::accept(GLActorVisitor &visitor, GLActor::VisitorAcceptRule)
		{
			return visitor.visit(this);
		}

		bool ComponentActor::accept(GLActorConstVisitor &visitor, GLActor::VisitorAcceptRule) const
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

		boost::shared_ptr<const Mantid::Geometry::CompAssembly> ComponentActor::getCompAssembly() const
		{
			return boost::dynamic_pointer_cast<const Mantid::Geometry::CompAssembly>(getComponent());
		}

		/**
		* A component is a non-detector if it's an ObjComponent (has a shape) and not an ObjCompAssembly
		* (a single object) and not a RectangularDetector (which is an assembly) or a StructuredDetector
		(which is an assembly).
		*/
		bool ComponentActor::isNonDetector() const
		{
			auto obj = getObjComponent();
			return  obj &&
				!getObjCompAssembly() &&
				!getDetector() &&
				!boost::dynamic_pointer_cast<const Mantid::Geometry::RectangularDetector>(obj) &&
				!boost::dynamic_pointer_cast<const Mantid::Geometry::StructuredDetector>(obj);
		}

	}//MantidWidgets
}//MantidQt

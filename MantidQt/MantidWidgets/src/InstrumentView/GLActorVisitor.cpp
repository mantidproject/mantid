#include "MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActor.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActorCollection.h"
#include "MantidQtMantidWidgets/InstrumentView/ComponentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/CompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/ObjCompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/RectangularDetectorActor.h"
#include "MantidQtMantidWidgets/InstrumentView/StructuredDetectorActor.h"

namespace MantidQt
{
	namespace MantidWidgets
	{
		//   Default visit implementations just call visit(GLActor*)

		bool GLActorVisitor::visit(GLActorCollection* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorVisitor::visit(CompAssemblyActor* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorVisitor::visit(ObjCompAssemblyActor* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorVisitor::visit(ComponentActor* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorVisitor::visit(
			InstrumentActor *actor) {
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorVisitor::visit(RectangularDetectorActor* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}
		
		bool GLActorVisitor::visit(StructuredDetectorActor* actor)
		{
			return this->visit(static_cast<GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const GLActorCollection* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const CompAssemblyActor* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const ObjCompAssemblyActor* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const ComponentActor* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(
			const InstrumentActor *actor) {
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const RectangularDetectorActor* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool GLActorConstVisitor::visit(const StructuredDetectorActor* actor)
		{
			return this->visit(static_cast<const GLActor*>(actor));
		}

		bool SetAllVisibleVisitor::visit(GLActor* actor)
		{
			actor->setVisibility(true);
			return true;
		}

		bool SetAllVisibleVisitor::visit(ComponentActor *actor)
		{
			bool on = (!actor->isNonDetector()) || m_showNonDet;
			actor->setVisibility(on);
			return true;
		}
	}//MantidWidgets
}//MantidQt


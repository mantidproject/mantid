#include "MantidQtWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtWidgets/InstrumentView/GLActor.h"
#include "MantidQtWidgets/InstrumentView/GLActorCollection.h"
#include "MantidQtWidgets/InstrumentView/ComponentActor.h"
#include "MantidQtWidgets/InstrumentView/CompAssemblyActor.h"
#include "MantidQtWidgets/InstrumentView/ObjCompAssemblyActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/RectangularDetectorActor.h"
#include "MantidQtWidgets/InstrumentView/StructuredDetectorActor.h"

namespace MantidQt {
namespace MantidWidgets {
//   Default visit implementations just call visit(GLActor*)

bool GLActorVisitor::visit(GLActorCollection *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(CompAssemblyActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(ObjCompAssemblyActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(ComponentActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(InstrumentActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(RectangularDetectorActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorVisitor::visit(StructuredDetectorActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const GLActorCollection *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const CompAssemblyActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const ObjCompAssemblyActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const ComponentActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const InstrumentActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const RectangularDetectorActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const StructuredDetectorActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool SetAllVisibleVisitor::visit(GLActor *actor) {
  actor->setVisibility(true);
  return true;
}

bool SetAllVisibleVisitor::visit(ComponentActor *actor) {
  bool on = (!actor->isNonDetector()) || m_showNonDet;
  actor->setVisibility(on);
  return true;
}
} // MantidWidgets
} // MantidQt

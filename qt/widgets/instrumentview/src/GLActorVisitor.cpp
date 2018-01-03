#include "MantidQtWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtWidgets/InstrumentView/GLActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

namespace MantidQt {
namespace MantidWidgets {
//   Default visit implementations just call visit(GLActor*)

bool GLActorVisitor::visit(InstrumentActor *actor) {
  return this->visit(static_cast<GLActor *>(actor));
}

bool GLActorConstVisitor::visit(const InstrumentActor *actor) {
  return this->visit(static_cast<const GLActor *>(actor));
}

bool SetAllVisibleVisitor::visit(GLActor *actor) {
  actor->setVisibility(true);
  return true;
}
} // MantidWidgets
} // MantidQt

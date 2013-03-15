#include "GLActorVisitor.h"

bool SetAllVisibleVisitor::visit(GLActor* actor)
{
  actor->setVisibility(true);
  return true;
}

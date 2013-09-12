#include "GLActorVisitor.h"

bool SetAllVisibleVisitor::visit(GLActor* actor)
{
  actor->setVisibility(true);
  return true;
}

bool SetAllVisibleVisitor::visit(ComponentActor *actor)
{
    bool on = (!actor->isNonDetector()) || m_showNonDet;
    actor->setVisibility( on );
}

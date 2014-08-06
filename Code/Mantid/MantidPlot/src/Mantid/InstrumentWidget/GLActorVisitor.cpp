#include "GLActorVisitor.h"
#include "GLActor.h"
#include "GLActorCollection.h"
#include "ComponentActor.h"
#include "CompAssemblyActor.h"
#include "ObjCompAssemblyActor.h"
#include "InstrumentActor.h"
#include "RectangularDetectorActor.h"

//   Default visit implementations just call visit(GLActor*)

bool GLActorVisitor::visit(GLActorCollection* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorVisitor::visit(CompAssemblyActor* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorVisitor::visit(ObjCompAssemblyActor* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorVisitor::visit(ComponentActor* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorVisitor::visit(InstrumentActor* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorVisitor::visit(RectangularDetectorActor* actor)
{
  return this->visit( static_cast<GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const GLActorCollection* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const CompAssemblyActor* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const ObjCompAssemblyActor* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const ComponentActor* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const InstrumentActor* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool GLActorConstVisitor::visit(const RectangularDetectorActor* actor)
{
  return this->visit( static_cast<const GLActor*>( actor ) );
}

bool SetAllVisibleVisitor::visit(GLActor* actor)
{
  actor->setVisibility(true);
  return true;
}

bool SetAllVisibleVisitor::visit(ComponentActor *actor)
{
    bool on = (!actor->isNonDetector()) || m_showNonDet;
    actor->setVisibility( on );
    return true;
}

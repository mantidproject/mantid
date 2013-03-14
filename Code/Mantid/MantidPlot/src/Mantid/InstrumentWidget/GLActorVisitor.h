#ifndef GLACTORVISITOR_H
#define GLACTORVISITOR_H

#include "GLActor.h"

/**
 * A base class for an actor visitor.
 */
class GLActorVisitor
{
public:
  /// Virtual destructor.
  virtual ~GLActorVisitor(){}
  /// Abstract method that must be implemented in sub-classes
  virtual bool visit(GLActor*) = 0;
};

/*
 * The visit() method implemented by sub-classes must return true if an actor 
 * is set visible and false otherwise. This is requered by GLActorCollection::accept()
 * method to determine whether the collection itself is visible or not.
 *
 * All visitors changing visibility should be sub-classed from this base class.
 */
class SetVisibilityVisitor: public GLActorVisitor
{
};

/**
 * Set all actors visible.
 */
class SetAllVisibleVisitor: public SetVisibilityVisitor
{
public:
  bool visit(GLActor*);
};

#endif // GLACTORVISITOR_H
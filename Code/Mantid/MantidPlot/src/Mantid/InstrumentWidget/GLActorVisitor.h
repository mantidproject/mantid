#ifndef GLACTORVISITOR_H
#define GLACTORVISITOR_H

class GLActor;
class GLActorCollection;
class ComponentActor;
class CompAssemblyActor;
class ObjCompAssemblyActor;
class InstrumentActor;
class RectangularDetectorActor;

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
    virtual bool visit(GLActorCollection*) = 0;
    virtual bool visit(CompAssemblyActor*) = 0;
    virtual bool visit(ObjCompAssemblyActor*) = 0;
    virtual bool visit(ComponentActor*) = 0;
    virtual bool visit(InstrumentActor*) = 0;
    virtual bool visit(RectangularDetectorActor*) = 0;
};

#define SAME_VISITS \
bool visit(GLActorCollection *actor ){return visit( (GLActor*) actor);}\
bool visit(CompAssemblyActor *actor){return visit( (GLActor*) actor);}\
bool visit(ObjCompAssemblyActor *actor){return visit( (GLActor*) actor);}\
bool visit(ComponentActor *actor){return visit( (GLActor*) actor);}\
bool visit(RectangularDetectorActor *actor){return visit( (GLActor*) actor);}\
bool visit(InstrumentActor *actor){return visit( (GLActor*) actor);}

#define SAME_VISITS_BUT_COMPONENTACTOR \
bool visit(GLActorCollection *actor ){return visit( (GLActor*) actor);}\
bool visit(CompAssemblyActor *actor){return visit( (GLActor*) actor);}\
bool visit(ObjCompAssemblyActor *actor){return visit( (GLActor*) actor);}\
bool visit(RectangularDetectorActor *actor){return visit( (GLActor*) actor);}\
bool visit(InstrumentActor *actor){return visit( (GLActor*) actor);}

/**
 * A base class for an actor visitor (const version).
 */
class GLActorConstVisitor
{
public:
  /// Virtual destructor.
  virtual ~GLActorConstVisitor(){}
  /// Abstract method that must be implemented in sub-classes
    virtual bool visit(const GLActor*) = 0;
    virtual bool visit(const GLActorCollection*) = 0;
    virtual bool visit(const CompAssemblyActor*) = 0;
    virtual bool visit(const ObjCompAssemblyActor*) = 0;
    virtual bool visit(const ComponentActor*) = 0;
    virtual bool visit(const InstrumentActor*) = 0;
    virtual bool visit(const RectangularDetectorActor*) = 0;
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
    SetAllVisibleVisitor(bool showNonDet):m_showNonDet(showNonDet){}
    bool visit(GLActor*);
    bool visit(ComponentActor *actor);
    SAME_VISITS_BUT_COMPONENTACTOR
private:
    bool m_showNonDet;
};

#endif // GLACTORVISITOR_H

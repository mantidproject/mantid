#ifndef GLACTORVISITOR_H
#define GLACTORVISITOR_H



namespace MantidQt
{
	namespace MantidWidgets
	{
		class GLActor;
		class GLActorCollection;
		class ComponentActor;
		class CompAssemblyActor;
		class ObjCompAssemblyActor;
		class RectangularDetectorActor;
		class StructuredDetectorActor;
		class InstrumentActor;


		/**
		* A base class for an actor visitor.
		*/
		class GLActorVisitor
		{
		public:
			/// Virtual destructor.
			virtual ~GLActorVisitor() {}
			/// Abstract method that must be implemented in sub-classes
			virtual bool visit(GLActor*) = 0;
			virtual bool visit(GLActorCollection*);
			virtual bool visit(CompAssemblyActor*);
			virtual bool visit(ObjCompAssemblyActor*);
			virtual bool visit(ComponentActor*);
			virtual bool visit(InstrumentActor*);
			virtual bool visit(RectangularDetectorActor*);
			virtual bool visit(StructuredDetectorActor*);
		};

		/**
		* A base class for an actor visitor (const version).
		*/
		class GLActorConstVisitor
		{
		public:
			/// Virtual destructor.
			virtual ~GLActorConstVisitor() {}
			/// Abstract method that must be implemented in sub-classes
			virtual bool visit(const GLActor*) = 0;
			virtual bool visit(const GLActorCollection*);
			virtual bool visit(const CompAssemblyActor*);
			virtual bool visit(const ObjCompAssemblyActor*);
			virtual bool visit(const ComponentActor*);
			virtual bool visit(const InstrumentActor*);
			virtual bool visit(const RectangularDetectorActor*);
			virtual bool visit(const StructuredDetectorActor *);
		};



		/*
		* The visit() method implemented by sub-classes must return true if an actor
		* is set visible and false otherwise. This is requered by GLActorCollection::accept()
		* method to determine whether the collection itself is visible or not.
		*
		* All visitors changing visibility should be sub-classed from this base class.
		*/
		class SetVisibilityVisitor : public GLActorVisitor
		{
		};

		/**
		* Set all actors visible.
		*/
		class SetAllVisibleVisitor : public SetVisibilityVisitor
		{
		public:
			explicit SetAllVisibleVisitor(bool showNonDet) : m_showNonDet(showNonDet) {}
			using GLActorVisitor::visit;
                        bool visit(GLActor *) override;
                        bool visit(ComponentActor *actor) override;

                private:
			bool m_showNonDet;
		};
	}//MantidWidgets
}//MantidQt


#endif // GLACTORVISITOR_H

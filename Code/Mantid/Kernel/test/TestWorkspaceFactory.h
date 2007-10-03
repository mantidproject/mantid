#include <cxxtest/TestSuite.h> 
#include "../inc/WorkspaceFactory.h" 

// Class Workspaces for testing purposes 
//class Workspace
//{
//	public:
//	virtual const std::string getID() const=0;
//};

class Work1 : public Workspace
{
	public:
	const std::string id() const {return "Work1";}
	static Workspace* create(){return new Work1;}
};

class Work2 : public Workspace
{
	public:
	const std::string id() const {return "Work2";}
	static Workspace* create(){return new Work2;}
};

class testWorkspaceFactory : public CxxTest::TestSuite
{
	private:
		Mantid::WorkspaceFactory* factory;
	public:
		testWorkspaceFactory()
		{
			factory=Mantid::WorkspaceFactory::Instance();
		}
		void testWFRegister()
		{
			TS_ASSERT(factory->registerWorkspace("Work1",&Work1::create));
		}
		void testWFUnregister()
		{
			TS_ASSERT(factory->unregisterWorkspace("Work1")); // Should work
			TS_ASSERT(!factory->unregisterWorkspace("Work1")); //Should not work the second time
		}	
		void testWFWorkspaceCreation()
		{
			factory->registerWorkspace("Work1",&Work1::create);
			Workspace* test=factory->createWorkspace("Work1");
			Work1* w1=dynamic_cast<Work1*>(test);
			TS_ASSERT(w1); //Created an object of type Work1.
		}						
		void testWFWorkspaceCreationThrows()
		{
		  TS_ASSERT_THROWS(Workspace* test=factory->createWorkspace("Dummy"),const std::runtime_error&);
		  TS_ASSERT_THROWS(Workspace* test=factory->createWorkspace(""),const std::runtime_error&);
		}
		void testWFWorkspaceRegisterThrows()
		{
			TS_ASSERT_THROWS(factory->registerWorkspace("",&Work1::create),const std::runtime_error&);
		}
			
};

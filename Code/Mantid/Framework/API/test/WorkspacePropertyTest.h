#ifndef WORKSPACEPROPERTYTEST_H_
#define WORKSPACEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceProperty.h"
#include <boost/shared_ptr.hpp>
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::MantidVec;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;

class WorkspacePropertyTest : public CxxTest::TestSuite
{


  class WorkspaceTester1 : public WorkspaceTester
  {
  public:
    const std::string id() const {return "WorkspacePropTest";}
  };

  // Second, identical private test class - used for testing check on workspace type in isValid()
  class WorkspaceTester2 : public WorkspaceTester
  {
  public:
    const std::string id() const {return "WorkspacePropTest";}
  };


public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspacePropertyTest *createSuite() { return new WorkspacePropertyTest(); }
  static void destroySuite( WorkspacePropertyTest *suite ) { delete suite; }

  WorkspacePropertyTest()
  {
    AnalysisDataService::Instance().clear();
    wsp1 = new WorkspaceProperty<Workspace>("workspace1","ws1",Direction::Input);
    wsp2 = new WorkspaceProperty<Workspace>("workspace2","",Direction::Output);
    wsp3 = new WorkspaceProperty<WorkspaceTester2>("workspace3","ws3",Direction::InOut);
    // Two optional properties of different types
    wsp4 = new WorkspaceProperty<Workspace>("workspace4","",Direction::Input, PropertyMode::Optional);
    wsp5 = new WorkspaceProperty<WorkspaceTester2>("workspace5","",Direction::Input, PropertyMode::Optional);
    wsp6 = new WorkspaceProperty<Workspace>("InvalidNameTest","",Direction::Output);
  }

  ~WorkspacePropertyTest()
  {
    delete wsp1;
    delete wsp2;
    delete wsp3;
    delete wsp4;
    delete wsp5;
    delete wsp6;
  }

  void testConstructor()
  {
    TS_ASSERT_THROWS( WorkspaceProperty<Workspace>("test","",3), std::out_of_range )
  }

  void testValue()
  {
    TS_ASSERT_EQUALS( wsp1->value(), "ws1" )
    TS_ASSERT_EQUALS( wsp2->value(), "" )
    TS_ASSERT_EQUALS( wsp3->value(), "ws3" )
  }

  void testSetValue()
  {
    TS_ASSERT_EQUALS( wsp1->setValue(""),
      "Enter a name for the Input/InOut workspace" )
    TS_ASSERT_EQUALS( wsp1->value(), "" )
    TS_ASSERT_EQUALS( wsp1->setValue("newValue"),
      "Workspace \"newValue\" was not found in the Analysis Data Service" )

    TS_ASSERT_EQUALS( wsp1->value(), "newValue" )
    wsp1->setValue("ws1");
  }

  void testSetValue_On_Optional()
  {
    TS_ASSERT_EQUALS(wsp4->setValue(""), "");
    TS_ASSERT_EQUALS(wsp4->value(), "");
    TS_ASSERT_EQUALS(wsp4->setValue("newValue"), 
		     "Workspace \"newValue\" was not found in the Analysis Data Service");
    TS_ASSERT_EQUALS( wsp4->value(), "newValue" );
    wsp4->setValue("");
  }

  void testIsValid()
  {  
    TS_ASSERT_EQUALS( wsp1->isValid(), "Workspace \"ws1\" was not found in the Analysis Data Service" );
    TS_ASSERT_EQUALS( wsp2->isValid(), "Enter a name for the Output workspace" );
    TS_ASSERT_EQUALS( wsp3->isValid(), "Workspace \"ws3\" was not found in the Analysis Data Service" );
    TS_ASSERT_EQUALS( wsp4->isValid(), "");
    TS_ASSERT_EQUALS( wsp6->isValid(), "Enter a name for the Output workspace");

    // Setting a valid workspace name should make wsp2 (an output workspace) valid
    TS_ASSERT_EQUALS( wsp2->setValue("ws2"), "" );
    TS_ASSERT_EQUALS( wsp2->isValid(), "" );
    // Setting an invalid name should make wsp6 invalid
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    AnalysisDataService::Instance().setIllegalCharacterList(illegalChars);
    std::string error = "Invalid object name 'ws6-1'. Names cannot contain any of the following characters: " + illegalChars;
    TS_ASSERT_EQUALS( wsp6->setValue("ws6-1"), error);
    TS_ASSERT_EQUALS( wsp6->isValid(), error);
    AnalysisDataService::Instance().setIllegalCharacterList("");

    WorkspaceFactory::Instance().subscribe<WorkspaceTester1>("WorkspacePropertyTest");
    WorkspaceFactory::Instance().subscribe<WorkspaceTester2>("WorkspacePropertyTest2");

    // The other three need the input workspace to exist in the ADS
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1) );
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("ws1", space) );
    wsp1->setValue("ws1");
    TS_ASSERT_EQUALS( wsp1->isValid(), "" );

    // Put workspace of wrong type and check validation fails
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("ws3", space) );
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS( wsp3->isValid(),
      "Workspace ws3 is not of the correct type" );
    // Now put correct type in and check it passes
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("WorkspacePropertyTest2",1,1,1) )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().addOrReplace("ws3", space) );
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS( wsp3->isValid(), "");

    // The optional one
    wsp4->setValue("ws1");
    TS_ASSERT_EQUALS( wsp4->isValid(), "" );
    // Check incorrect type
    wsp5->setValue("ws1");
    TS_ASSERT_EQUALS(wsp5->isValid(),
		     "Workspace ws1 is not of the correct type" );
    // Now the correct type
    wsp5->setValue("ws3");
    TS_ASSERT_EQUALS(wsp5->isValid(), "");
  }

  void testIsDefaultAndGetDefault()
  {
    //The constructor set wsp2 = "" so getDefault should always equal "", we'll change the value and check
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
    //change the value to something else anything
    wsp2->setValue("ws2");
    //it is not default now
    TS_ASSERT( !wsp2->isDefault() )
    //is default should stay the same
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
    wsp2->setValue("");
    TS_ASSERT( wsp2->isDefault() )
    TS_ASSERT_EQUALS(wsp2->getDefault(), "")
  }

  void testAllowedValues()
  {
    std::vector<std::string> vals;
    TS_ASSERT_THROWS_NOTHING( vals = wsp1->allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 2 )
    TS_ASSERT( std::find( vals.begin(), vals.end(), "ws1") != vals.end() )
    TS_ASSERT( std::find( vals.begin(), vals.end(), "ws3") != vals.end() )

    TS_ASSERT( wsp2->allowedValues().empty() )

    TS_ASSERT_THROWS_NOTHING( vals = wsp3->allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 1 )
  }

  void testCreateHistory()
  {
    PropertyHistory history = wsp1->createHistory();
    TS_ASSERT_EQUALS( history.name(), "workspace1" )
    TS_ASSERT_EQUALS( history.value(), "ws1" )
    TS_ASSERT( history.isDefault() )
    TS_ASSERT_EQUALS( history.type(), wsp1->type() )
    TS_ASSERT_EQUALS( history.direction(), 0 )

    //change the name back to ws2 to check that isDefault() fails
    wsp2->setValue("ws2");
    PropertyHistory history2 = wsp2->createHistory();
    TS_ASSERT_EQUALS( history2.name(), "workspace2" )
    TS_ASSERT_EQUALS( history2.value(), "ws2" )
    TS_ASSERT( ! history2.isDefault() )
    TS_ASSERT_EQUALS( history2.type(), wsp2->type() )
    TS_ASSERT_EQUALS( history2.direction(), 1 )

    // create empty workspace with blank name
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1) );
    auto wsp7 = new WorkspaceProperty<Workspace>("workspace7", "", Direction::Input);
    *wsp7 = space;
    TS_ASSERT(wsp7->getWorkspace())

    //test the history contains an empty name
    PropertyHistory history3 = wsp7->createHistory();
    TS_ASSERT_EQUALS( history3.name(), "workspace7" )
    TS_ASSERT( !history3.value().empty() )
    TS_ASSERT_EQUALS( history3.value().substr(0,5), "__TMP" )
    TS_ASSERT_EQUALS( history3.type(), wsp7->type() )
    TS_ASSERT_EQUALS( history3.direction(), 0 )
    wsp7->setValue("ws2");
    delete wsp7;
  }

  void testStore()
  {
    // This is an input workspace so should return false
    TS_ASSERT( ! wsp1->store() )

    // Since no workspace has been assigned to this output property, it should throw
    TS_ASSERT_THROWS( wsp2->store(), std::runtime_error )
    // So now create and assign the workspace and test again
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1) );
    *wsp2 = space;
    TS_ASSERT( wsp2->store() )
    // Check it really has been stored in the ADS
    Workspace_sptr storedspace;
    TS_ASSERT_THROWS_NOTHING( storedspace = AnalysisDataService::Instance().retrieve("ws2") )
    TS_ASSERT_EQUALS( storedspace->id(), "WorkspacePropTest" )

    // This one should pass
    TS_ASSERT( wsp3->store() )

    //Should be cleared as part of store so these should be empty
    TS_ASSERT( ! wsp1->operator()() )
    TS_ASSERT( ! wsp2->operator()() )
    TS_ASSERT( ! wsp3->operator()() )
  }

  void testTempName()
  {
    wsp4->setValue("");
    // Create and assign the workspace
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1) );
    *wsp4 = space;

    PropertyHistory history = wsp4->createHistory();
    TS_ASSERT( !history.value().empty() )
    TS_ASSERT_EQUALS( history.value().substr(0,5), "__TMP" )
  }

  void testDirection()
  {
    TS_ASSERT_EQUALS( wsp1->direction(), 0 );
    TS_ASSERT_EQUALS( wsp2->direction(), 1 );
    TS_ASSERT_EQUALS( wsp3->direction(), 2 );
    TS_ASSERT_EQUALS( wsp4->direction(), 0 );
    TS_ASSERT_EQUALS( wsp5->direction(), 0 );      
  }

  void test_locking()
  {
    // All the default ones are locking.
    TS_ASSERT( wsp1->isLocking());
    TS_ASSERT( wsp2->isLocking());
    TS_ASSERT( wsp3->isLocking());
    TS_ASSERT( wsp4->isLocking());
    TS_ASSERT( wsp5->isLocking());

    // Create one that is not locking
    WorkspaceProperty<Workspace> p1("workspace1","ws1",Direction::Input, PropertyMode::Mandatory, LockMode::NoLock);
    TS_ASSERT( !p1.isLocking());

    // Copy constructor, both ways
    WorkspaceProperty<Workspace> wsp1_copy(*wsp1);
    TS_ASSERT( wsp1_copy.isLocking());
    WorkspaceProperty<Workspace> p2(p1);
    TS_ASSERT( !p2.isLocking());

  }

  void test_storing_workspace_name_assing()
  {
      Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1);
      AnalysisDataService::Instance().add("space1", ws1);
      WorkspaceProperty<Workspace> p1("workspace1","",Direction::Input);
      p1 = ws1;
      TS_ASSERT_EQUALS( p1.value(), "space1" );
      AnalysisDataService::Instance().clear();
  }

  void test_storing_workspace_name_setDataItem()
  {
      Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1);
      AnalysisDataService::Instance().add("space1", ws1);
      WorkspaceProperty<Workspace> p1("workspace1","",Direction::Input);
      p1.setDataItem( ws1 );
      TS_ASSERT_EQUALS( p1.value(), "space1" );
      AnalysisDataService::Instance().clear();
  }

  void test_not_storing_workspace_name()
  {
      Workspace_sptr ws1 = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1);
      WorkspaceProperty<Workspace> p1("workspace1","",Direction::Input);
      p1 = ws1;
      TS_ASSERT_EQUALS( p1.value(), "" );
  }

private:
  WorkspaceProperty<Workspace> *wsp1;
  WorkspaceProperty<Workspace> *wsp2;
  WorkspaceProperty<WorkspaceTester2> *wsp3;
  WorkspaceProperty<Workspace> *wsp4;
  WorkspaceProperty<WorkspaceTester2> *wsp5;
  WorkspaceProperty<Workspace> *wsp6;
};

#endif /*WORKSPACEPROPERTYTEST_H_*/

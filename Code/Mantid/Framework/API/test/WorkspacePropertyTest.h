#ifndef WORKSPACEPROPERTYTEST_H_
#define WORKSPACEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceProperty.h"
#include <boost/shared_ptr.hpp>
#include "MantidAPI/WorkspaceFactory.h"

using Mantid::MantidVec;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;

class WorkspacePropertyTest : public CxxTest::TestSuite
{

  //private test class - using this removes the dependency on the DataObjects library
  class WorkspaceTest: public MatrixWorkspace
  {
  public:
    virtual size_t getNumberHistograms() const { return 1;}

    const std::string id() const {return "WorkspacePropTest";}
    //section required to support iteration
    virtual size_t size() const {return 0;}
    virtual size_t blocksize() const  {return 1000000;}
    virtual MantidVec& dataX(size_t const) {return data;}
    ///Returns the y data
    virtual MantidVec& dataY(size_t const) {return data;}
    ///Returns the error data
    virtual MantidVec& dataE(size_t const) {return data;}
    ///Returns the x error data
    virtual MantidVec& dataDx(int const) {return data;}

    virtual const MantidVec& dataX(size_t const)const {return data;}
    ///Returns the y data
    virtual const MantidVec& dataY(size_t const )const {return data;}
    ///Returns the error data
    virtual const MantidVec& dataE(int const)const {return data;}
    ///Returns the x error data
    virtual const MantidVec& dataDx(int const)const {return data;}
    cow_ptr<MantidVec> refX(const int) const {return cow_ptr<MantidVec>();}
    void setX(const int, const cow_ptr<MantidVec>&) {}
    virtual void init(const int &, const int &, const int &){};

  private:
    MantidVec data;
    int dummy;
  };

  // Second, identical private test class - used for testing check on workspace type in isValid()
  class WorkspaceTest2 : public MatrixWorkspace
  {
  public:
    virtual size_t getNumberHistograms() const { return 1;}

    const std::string id() const {return "WorkspacePropTest";}
    //section required to support iteration
    virtual size_t size() const {return 0;}
    virtual size_t blocksize() const  {return 1000000;}
    virtual MantidVec& dataX(size_t const) {return data;}
    ///Returns the y data
    virtual MantidVec& dataY(size_t const) {return data;}
    ///Returns the error data
    virtual MantidVec& dataE(size_t const) {return data;}
    ///Returns the x error data
    virtual MantidVec& dataDx(int const) {return data;}

    virtual const MantidVec& dataX(size_t const)const {return data;}
    ///Returns the y data
    virtual const MantidVec& dataY(size_t const)const {return data;}
    ///Returns the error data
    virtual const MantidVec& dataE(int const)const {return data;}
    ///Returns the x error data
    virtual const MantidVec& dataDx(int const)const {return data;}
    cow_ptr<MantidVec> refX(const int) const {return cow_ptr<MantidVec>();}
    void setX(const int, const cow_ptr<MantidVec>&) {}
    virtual void init(const int &, const int &, const int &){};

  private:
    MantidVec data;
    int dummy;
  };

public:
  WorkspacePropertyTest()
  {
    wsp1 = new WorkspaceProperty<Workspace>("workspace1","ws1",Direction::Input);
    wsp2 = new WorkspaceProperty<Workspace>("workspace2","",Direction::Output);
    wsp3 = new WorkspaceProperty<WorkspaceTest2>("workspace3","ws3",Direction::InOut);
    // Two optional properties of different types
    wsp4 = new WorkspaceProperty<Workspace>("workspace4","",Direction::Input, true);
    wsp5 = new WorkspaceProperty<WorkspaceTest2>("workspace5","",Direction::Input, true);
  }

  ~WorkspacePropertyTest()
  {
    delete wsp1;
    delete wsp2;
    delete wsp3;
    delete wsp4;
    delete wsp5;
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

    // Setting the workspace name should make wsp2 (an output workspace) valid
    TS_ASSERT_EQUALS( wsp2->setValue("ws2"), "" );
    TS_ASSERT_EQUALS( wsp2->isValid(), "" );

    WorkspaceFactory::Instance().subscribe<WorkspaceTest>("WorkspacePropertyTest");
    WorkspaceFactory::Instance().subscribe<WorkspaceTest2>("WorkspacePropertyTest2");

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
    std::set<std::string> vals;
    TS_ASSERT_THROWS_NOTHING( vals = wsp1->allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 2 )
    TS_ASSERT( vals.count("ws1") )
    TS_ASSERT( vals.count("ws3") )

    TS_ASSERT( wsp2->allowedValues().empty() )

    TS_ASSERT_THROWS_NOTHING( vals = wsp3->allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 2 )
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

    PropertyHistory history3 = wsp3->createHistory();
    TS_ASSERT_EQUALS( history3.name(), "workspace3" )
    TS_ASSERT_EQUALS( history3.value(), "ws3" )
    TS_ASSERT( history3.isDefault() )
    TS_ASSERT_EQUALS( history3.type(), wsp3->type() )
    TS_ASSERT_EQUALS( history3.direction(), 2 )

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
    TS_ASSERT( ! storedspace->id().compare("WorkspacePropTest") )

    // This one should pass
    TS_ASSERT( wsp3->store() )

    //Should be cleared as part of store so these should be empty
    TS_ASSERT( ! wsp1->operator()() )
    TS_ASSERT( ! wsp2->operator()() )
    TS_ASSERT( ! wsp3->operator()() )
  }

  void testDirection()
  {
    TS_ASSERT_EQUALS( wsp1->direction(), 0 );
    TS_ASSERT_EQUALS( wsp2->direction(), 1 );
    TS_ASSERT_EQUALS( wsp3->direction(), 2 );
    TS_ASSERT_EQUALS( wsp4->direction(), 0 );
    TS_ASSERT_EQUALS( wsp5->direction(), 0 );      
  }

private:
  WorkspaceProperty<Workspace> *wsp1;
  WorkspaceProperty<Workspace> *wsp2;
  WorkspaceProperty<WorkspaceTest2> *wsp3;
  WorkspaceProperty<Workspace> *wsp4;
  WorkspaceProperty<WorkspaceTest2> *wsp5;
};

#endif /*WORKSPACEPROPERTYTEST_H_*/

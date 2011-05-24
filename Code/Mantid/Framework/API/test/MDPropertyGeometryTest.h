#ifndef MD_PROPERTY_CPR_H
#define MD_PROPERTY_CPR_H

#include <cxxtest/TestSuite.h>
#include <sstream>
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDWorkspace.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "MantidAPI/MDPropertyGeometry.h"
#include "MantidAPI/WorkspaceFactory.h"

using Mantid::MantidVec;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class MDPropertyGeometryTest : public CxxTest::TestSuite
{

  
  class PropertyManagerHelper : public PropertyManager
  {
    public:
    PropertyManagerHelper() : PropertyManager() {}

      using PropertyManager::declareProperty;
      using PropertyManager::setProperty;
      using PropertyManager::getPointerToProperty;
  };
  
  //Helper constructional method.
  static Mantid::Geometry::MDGeometry* constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 0));
    basisDimensions.insert(MDBasisDimension("q2", true, 1));
    basisDimensions.insert(MDBasisDimension("q3", true, 2));
    basisDimensions.insert(MDBasisDimension("u1", false, 3));

    boost::shared_ptr<OrientedLattice> spCell = boost::shared_ptr<OrientedLattice>(new OrientedLattice());
    return new MDGeometry(MDGeometryBasis(basisDimensions, spCell));
  }

public:
  MDPropertyGeometryTest():wsp1(NULL),wsp2(NULL)
  {
   // wsp2 = new WorkspaceProperty<Workspace>("workspace2","",Direction::Output);
    //wsp3 = new WorkspaceProperty<WorkspaceTest2>("workspace3","ws3",Direction::InOut);
  }

  ~MDPropertyGeometryTest()
  {
    delete wsp1;
    //delete wsp2;
    //delete wsp3;
  }

  void testConstructor()
  {
    TS_ASSERT_THROWS_NOTHING(wsp1 = new MDPropertyGeometry("geometryDescription","ws1",Direction::Input));
    TS_ASSERT_EQUALS( wsp1->value(), "ws1" );
//    TS_ASSERT_THROWS( WorkspaceProperty<Workspace>("test","",3), std::out_of_range )
  }
  void testServices(){
      boost::scoped_ptr<MDGeometry> pGeom(constructMDGeometry());
      
      TS_ASSERT_THROWS_NOTHING(manager.declareProperty( new MDPropertyGeometry("geometryDescription","ws1",Direction::Input),"this property describes the geometry obtained from string"));
      manager.declareProperty( new MDPropertyGeometry("geom2Description",*pGeom,Direction::Input),"this property describes the geometry obtained from object");
      TS_ASSERT_EQUALS(manager.existsProperty("geometryDescription"),true);
      const std::vector< Property*> prop = manager.getProperties();
      // property is indeed the MD property 
      TS_ASSERT_THROWS_NOTHING(wsp2=dynamic_cast<MDPropertyGeometry *>(prop[0]));
      if(wsp2==NULL){
          TS_FAIL("workspace property has not been casted to MDProperty geometry");
      }

	  TS_ASSERT_THROWS_NOTHING(wsp2->pDimDescription(0)->cut_min=10);
      
  }

  void testMDGeometryDescriptionAccess()
  {
   if(!wsp2){
       TS_FAIL("workspace property has not been casted to MDProperty geometry");
   }else{
     TS_ASSERT_THROWS_NOTHING(wsp2->getNumDims());
     TS_ASSERT_THROWS_NOTHING(wsp2->pDimDescription(0)->nBins = 100);
     TS_ASSERT_THROWS_NOTHING(wsp2->pDimDescription("q3")->nBins=200);
     TS_ASSERT_EQUALS(wsp2->pDimDescription(0)->nBins,100);
     TS_ASSERT_EQUALS(wsp2->pDimDescription(2)->nBins,200);
   }
  }

  void testIOOperations(){
      if(!wsp2)TS_FAIL("workspace property has not been casted to MDPropertyGeometry");

       std::stringstream buf;
       TS_ASSERT_THROWS_NOTHING(buf << (*wsp2));
       TS_ASSERT_EQUALS(buf.str(),"TEST PROPERTY");
       TS_ASSERT_THROWS_NOTHING((buf>>(*wsp2)));

  }
 
  void testSetValue()
  {
    TS_ASSERT_THROWS_NOTHING(wsp2->setValue(" should be something meaningful which is not implemented yet"));
    TS_ASSERT_THROWS_NOTHING(wsp1->setValue(*wsp2));

/*
    TS_ASSERT_EQUALS( wsp1->setValue(""),
      "Enter a name for the workspace" )
    TS_ASSERT_EQUALS( wsp1->value(), "" )
    TS_ASSERT_EQUALS( wsp1->setValue("newValue"),
      "Workspace \"newValue\" was not found in the Analysis Data Service" )

    TS_ASSERT_EQUALS( wsp1->value(), "newValue" )
    wsp1->setValue("ws1");
*/
  }
/*
  void t__IsValid()
  {  
    TS_ASSERT_EQUALS( wsp1->isValid(), "Workspace \"ws1\" was not found in the Analysis Data Service" )
    TS_ASSERT_EQUALS( wsp2->isValid(), "Enter a name for the workspace" )
    TS_ASSERT_EQUALS( wsp3->isValid(), "Workspace \"ws3\" was not found in the Analysis Data Service" )

    // Setting the workspace name should make wsp2 (an output workspace) valid
    TS_ASSERT_EQUALS( wsp2->setValue("ws2"), "" )
    TS_ASSERT_EQUALS( wsp2->isValid(), "" )

    WorkspaceFactory::Instance().subscribe<WorkspaceTest>("WorkspacePropertyTest");
    WorkspaceFactory::Instance().subscribe<WorkspaceTest2>("WorkspacePropertyTest2");

    // The other two need the input workspace to exist in the ADS
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(space = WorkspaceFactory::Instance().create("WorkspacePropertyTest",1,1,1) );
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("ws1", space) );
    wsp1->setValue("ws1");
    TS_ASSERT_EQUALS( wsp1->isValid(), "" )

    // Put workspace of wrong type and check validation fails
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("ws3", space) );
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS( wsp3->isValid(),
      "Workspace ws3 is not of the correct type" );
    // Now put correct type in and check it passes
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("WorkspacePropertyTest2",1,1,1) )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().addOrReplace("ws3", space) );
    wsp3->setValue("ws3");
    TS_ASSERT_EQUALS( wsp3->isValid(), "")
  }

  void t__IsDefaultAndGetDefault()
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

  void t__AllowedValues()
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

  void t__CreateHistory()
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

  void t__Store()
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

  void t__Direction()
  {
    TS_ASSERT_EQUALS( wsp1->direction(), 0 )
    TS_ASSERT_EQUALS( wsp2->direction(), 1 )
    TS_ASSERT_EQUALS( wsp3->direction(), 2 )
  }
*/
private:
  MDPropertyGeometry *wsp1;
  MDPropertyGeometry *wsp2;
  PropertyManagerHelper manager;

  //WorkspaceProperty<Workspace> *wsp2;
 // WorkspaceProperty<WorkspaceTest2> *wsp3;


};

#endif /*WORKSPACEPROPERTYTEST_H_*/

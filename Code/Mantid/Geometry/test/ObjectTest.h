#ifndef MANTID_TESTOBJECT__
#define MANTID_TESTOBJECT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "MantidGeometry/V3D.h" 
#include "MantidGeometry/Object.h" 
#include "MantidGeometry/Cylinder.h" 
#include "MantidGeometry/Sphere.h" 
#include "MantidGeometry/Plane.h" 
#include "MantidGeometry/Algebra.h" 
#include "MantidGeometry/SurfaceFactory.h" 
#include "MantidGeometry/Track.h" 

using namespace Mantid;
using namespace Geometry;

class testObject: public CxxTest::TestSuite
{
  private:


public:


  void testSetObject1()
  {
    Object A;
    std::string Ostr="18 45 #(45 (57 : 56))";
    A.setObject(5,Ostr);
    TS_ASSERT_EQUALS(A.cellStr(MObj),"#( (57 : 56) 45 ) 45 18");
  }

  void testSetObject2() 
    /*!
    Test the porcessing of a line
    \retval -1 :: Unable to process line
    \retval 0 :: success
    */
  {
    Object A;
    std::string Ostr="-5  8  60  -61  62  -63 #3";
    A.setObject(4,Ostr);
    TS_ASSERT_EQUALS(A.hasComplement(),1);
    //id=4 material=-1 density=0
    TS_ASSERT_EQUALS(A.str(),"4 -1 0 #3 -63 62 -61 60 8 -5");
  }

  void testCellStr()
    /*!
    Test the CellSTr method including the 
    complementary of both #(XXX) and #Cell.
    */
  {
    populateMObj();
    Object A;
    std::string Ostr="-5  8  60  (-61 :  62)  -63 #3";
    A.setObject(4,Ostr);
    TS_ASSERT_EQUALS(A.cellStr(MObj),"#(-60006 60005 -60004 60003 -60002 60001)  -63 (-61 : 62) 60 8 -5");
  }

  void testComplement() 
    /*!
    Test the removal of a complement
    \retval -1 :: Unable to process line
    \retval 0 :: success
    */
  {
    populateMObj();

    Object A;
    std::string Ostr="1 -2 3 -4 5 -6  #10";
    A.setObject(4, Ostr);
    TS_ASSERT_EQUALS(A.hasComplement(),1);

    Algebra AX;
    AX.setFunctionObjStr(A.cellStr(MObj));

    if (!A.procString(AX.writeMCNPX()))
    {
      TS_FAIL("Failed Complement ");
    }
    TS_ASSERT_EQUALS(A.str(),"4 -1 0 (-80001 : -80005 : 80006 : ((-80003 : 80002) 80004)) 1 3 5 -6 -4 -2");
  }

  void testMakeComplement()
    /*!
    Test the making of a given object complementary
    */
  {
    populateMObj();

    TS_ASSERT_EQUALS(MObj[2].str(),"2 -1 0 -63 62 -61 60 5 -4");
    MObj[2].makeComplement();
    TS_ASSERT_EQUALS(MObj[2].str(),"2 -1 0 #( -63 62 -61 60 5 -4 )");
  }

  void testIsOnSideCappedCylinder()
  {
    Object A = createCappedCylinder();
    //inside
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,0)),0); //origin
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,2.9,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-2.9,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-2.9)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,2.9)),0);
    //on the side
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-3)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,3)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.2,0,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.2,0,0)),1);

    //on the edges
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.2,3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.2,-3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.2,0,-3)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.2,0,3)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.2,3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.2,-3,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.2,0,-3)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.2,0,3)),1);
    //out side
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,3.1,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-3.1,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-3.1)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,3.1)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(1.3,0,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(-3.3,0,0)),0);
  }

  void testIsValidCappedCylinder()
  {
    Object A = createCappedCylinder();
    //inside
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,0)),1); //origin
    TS_ASSERT_EQUALS(A.isValid(V3D(0,2.9,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-2.9,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-2.9)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,2.9)),1);
    //on the side
    TS_ASSERT_EQUALS(A.isValid(V3D(0,3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-3)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,3)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.2,0,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,0,0)),1);

    //on the edges
    TS_ASSERT_EQUALS(A.isValid(V3D(1.2,3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.2,-3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.2,0,-3)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.2,0,3)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,-3,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,0,-3)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,0,3)),1);
    //out side
    TS_ASSERT_EQUALS(A.isValid(V3D(0,3.1,0)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-3.1,0)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-3.1)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,3.1)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.3,0,0)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(-3.3,0,0)),0);
  }

  void testIsOnSideSphere()
  {
    Object A = createSphere();
    //inside
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,0)),0); //origin
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,4.0,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-4.0,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-4.0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,4.0)),0);
    //on the side
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,4.1,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-4.1,0)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-4.1)),1);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,4.1)),1);

    //out side
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,4.2,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,-4.2,0)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,-4.2)),0);
    TS_ASSERT_EQUALS(A.isOnSide(V3D(0,0,4.2)),0);
  }

  void testIsValidSphere()
  {
    Object A = createSphere();
    //inside
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,0)),1); //origin
    TS_ASSERT_EQUALS(A.isValid(V3D(0,4.0,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-4.0,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-4.0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,4.0)),1);
    //on the side
    TS_ASSERT_EQUALS(A.isValid(V3D(0,4.1,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-4.1,0)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-4.1)),1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,4.1)),1);

    //out side
    TS_ASSERT_EQUALS(A.isValid(V3D(0,4.2,0)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,-4.2,0)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,-4.2)),0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0,0,4.2)),0);
  }

  void testCalcValidTypeSphere()
  {
    Object A = createSphere();
    //entry on the normal
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-4.1,0,0),V3D(1,0,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-4.1,0,0),V3D(-1,0,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(4.1,0,0),V3D(1,0,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(4.1,0,0),V3D(-1,0,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,-4.1,0),V3D(0,1,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,-4.1,0),V3D(0,-1,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,4.1,0),V3D(0,1,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,4.1,0),V3D(0,-1,0)),1);

    //a glancing blow
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-4.1,0,0),V3D(0,1,0)),0);
    //not quite on the normal
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-4.1,0,0),V3D(0.5,0.5,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(4.1,0,0),V3D(0.5,0.5,0)),-1);
  }

  void testCalcValidTypeCappedCylinder()
  {
    Object A = createCappedCylinder();
    //entry on the normal
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-3.2,0,0),V3D(1,0,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-3.2,0,0),V3D(-1,0,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(1.2,0,0),V3D(1,0,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(1.2,0,0),V3D(-1,0,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,-3,0),V3D(0,1,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,-3,0),V3D(0,-1,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,3,0),V3D(0,1,0)),-1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(0,3,0),V3D(0,-1,0)),1);

    //a glancing blow
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-3.2,0,0),V3D(0,1,0)),0);
    //not quite on the normal
    TS_ASSERT_EQUALS(A.calcValidType(V3D(-3.2,0,0),V3D(0.5,0.5,0)),1);
    TS_ASSERT_EQUALS(A.calcValidType(V3D(1.2,0,0),V3D(0.5,0.5,0)),-1);
  }

  void testInterceptSurfaceSphereZ()
  {
    std::vector<TUnit> expectedResults;
    std::string S41="s 1 1 1 4";         // Sphere at (1,1,1) radius 4

    // First create some surfaces
    std::map<int,Surface*> SphSurMap;
    SphSurMap[41]=new Sphere();
    SphSurMap[41]->setSurface(S41);
    SphSurMap[41]->setName(41);

    // A sphere 
    std::string ObjSphere="-41" ;

    Object A; 
    A.setObject(41,ObjSphere);
    A.populate(SphSurMap);


    Track track(V3D(-1,1.5,1),V3D(1,0,0));

    // format = startPoint, endPoint, total distance so far, objectID
	// forward only intercepts means that start point should be track origin
    //expectedResults.push_back(TUnit(V3D(-sqrt(16-0.25)+1,1.5,1),
    expectedResults.push_back(TUnit(V3D(-1,1.5,1),
				    V3D(sqrt(16-0.25)+1,1.5,1.0),sqrt(15.75)+2,A.getName()));

    checkTrackIntercept(A,track,expectedResults);
  }

  void testInterceptSurfaceSphereY()
  {
    std::vector<TUnit> expectedResults;
    Object A = createSphere();
    Track track(V3D(0,-10,0),V3D(0,1,0));

    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(0,-4.1,0),V3D(0,4.1,0),14.1,A.getName()));

    checkTrackIntercept(A,track,expectedResults);
  }

  void testInterceptSurfaceSphereX()
  {
    std::vector<TUnit> expectedResults;
    Object A = createSphere();
    Track track(V3D(-10,0,0),V3D(1,0,0));
    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(-4.1,0,0),V3D(4.1,0,0),14.1,A.getName()));
    checkTrackIntercept(A,track,expectedResults);
  }

  void testInterceptSurfaceCappedCylinderY()
  {
    std::vector<TUnit> expectedResults;
    Object A = createCappedCylinder();
    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(0,-3,0),V3D(0,3,0),13,A.getName()));

    Track track(V3D(0,-10,0),V3D(0,1,0));
    checkTrackIntercept(A,track,expectedResults);
  }

  void testInterceptSurfaceCappedCylinderX()
  {
    std::vector<TUnit> expectedResults;
    Object A = createCappedCylinder();
    Track track(V3D(-10,0,0),V3D(1,0,0));

    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(-3.2,0,0),V3D(1.2,0,0),11.2,A.getName()));

    checkTrackIntercept(A,track,expectedResults);
  }
  
  void testInterceptSurfaceCappedCylinderMiss()
  {
    std::vector<TUnit> expectedResults; //left empty as there are no expected results
    Object A = createCappedCylinder();
    Track track(V3D(-10,0,0),V3D(1,1,0));

    checkTrackIntercept(A,track,expectedResults);
  }

  void checkTrackIntercept(Track& track, std::vector<TUnit>& expectedResults)
  {
    int index = 0;
    for (Track::LType::const_iterator it = track.begin(); it!=track.end();++it)
    {
      TS_ASSERT_DELTA(it->Dist,expectedResults[index].Dist,1e-6);
      TS_ASSERT_DELTA(it->Length,expectedResults[index].Length,1e-6);
      TS_ASSERT_EQUALS(it->ObjID,expectedResults[index].ObjID);
      TS_ASSERT_EQUALS(it->PtA,expectedResults[index].PtA);
      TS_ASSERT_EQUALS(it->PtB,expectedResults[index].PtB);
      ++index;
    }
    TS_ASSERT_EQUALS(index,static_cast<int>(expectedResults.size()));
  }

  void checkTrackIntercept(Object& obj, Track& track, std::vector<TUnit>& expectedResults)
    {
      int unitCount = obj.interceptSurface(track);
      TS_ASSERT_EQUALS(unitCount,expectedResults.size())
      checkTrackIntercept(track,expectedResults);
    }

void testTrackTwoIsolatedCubes()
  /*!
    Test a track going through an object
   */
{
  std::string ObjA="60001 -60002 60003 -60004 60005 -60006";
  std::string ObjB="80001 -80002 60003 -60004 60005 -60006";
  MObj.erase(MObj.begin(),MObj.end());
  MObj[3]=Object();
  MObj[3].setObject(3,ObjA);

  MObj[4]=Object();
  MObj[4].setObject(4,ObjB);

  createSurfaces();
  
  Track TL(Geometry::V3D(-5,0,0),
	   Geometry::V3D(1,0,0));

  // CARE: This CANNOT be called twice
  TS_ASSERT(MObj[3].interceptSurface(TL)!=0)
  TS_ASSERT(MObj[4].interceptSurface(TL)!=0)

  std::vector<TUnit> expectedResults;
  expectedResults.push_back(TUnit(V3D(-1,0,0),V3D(1,0,0),6,3));
  expectedResults.push_back(TUnit(V3D(4.5,0,0),V3D(6.5,0,0),11.5,4));
  checkTrackIntercept(TL,expectedResults);

}

void testTrackTwoTouchingCubes()
  /*!
    Test a track going through an object
   */
{
  std::string ObjA="60001 -60002 60003 -60004 60005 -60006";
  std::string ObjB="60002 -80002 60003 -60004 60005 -60006";
  MObj.erase(MObj.begin(),MObj.end());
  MObj[3]=Object();
  MObj[3].setObject(3,ObjA);

  MObj[4]=Object();
  MObj[4].setObject(4,ObjB);

  createSurfaces();
  
  Track TL(Geometry::V3D(-5,0,0),
	   Geometry::V3D(1,0,0));

  // CARE: This CANNOT be called twice
  TS_ASSERT(MObj[3].interceptSurface(TL)!=0)
  TS_ASSERT(MObj[4].interceptSurface(TL)!=0)

  std::vector<TUnit> expectedResults;
  expectedResults.push_back(TUnit(V3D(-1,0,0),V3D(1,0,0),6,3));
  expectedResults.push_back(TUnit(V3D(1,0,0),V3D(6.5,0,0),11.5,4));

  checkTrackIntercept(TL,expectedResults);
}

void testTrackCubeWithInternalSphere()
  /*!
    Test a track going through an object
  */
{
  std::string ObjA="60001 -60002 60003 -60004 60005 -60006 71";
  std::string ObjB="-71";
  MObj.erase(MObj.begin(),MObj.end());
  MObj[3]=Object();
  MObj[3].setObject(3,ObjA);

  MObj[4]=Object();
  MObj[4].setObject(4,ObjB);

  createSurfaces();
  
  Track TL(Geometry::V3D(-5,0,0),
	   Geometry::V3D(1,0,0));

  // CARE: This CANNOT be called twice
  TS_ASSERT(MObj[3].interceptSurface(TL)!=0);
  TS_ASSERT(MObj[4].interceptSurface(TL)!=0);

  std::vector<TUnit> expectedResults;
  expectedResults.push_back(TUnit(V3D(-1,0,0),V3D(-0.8,0,0),4.2,3));
  expectedResults.push_back(TUnit(V3D(-0.8,0,0),V3D(0.8,0,0),5.8,4));
  expectedResults.push_back(TUnit(V3D(0.8,0,0),V3D(1,0,0),6,3));
  checkTrackIntercept(TL,expectedResults);

}

void testTrack_CubePlusInternalEdgeTouchSpheres()
  /*!
    Test a track going through an object
  */
{
  std::string ObjA="60001 -60002 60003 -60004 60005 -60006 72 73";
  std::string ObjB="(-72 : -73)";
  MObj.erase(MObj.begin(),MObj.end());
  MObj[3]=Object();
  MObj[3].setObject(3,ObjA);

  MObj[4]=Object();
  MObj[4].setObject(4,ObjB);

  createSurfaces();
  
  Track TL(Geometry::V3D(-5,0,0),
	   Geometry::V3D(1,0,0));


  // CARE: This CANNOT be called twice
  TS_ASSERT(MObj[3].interceptSurface(TL)!=0);
  TS_ASSERT(MObj[4].interceptSurface(TL)!=0);

  std::vector<TUnit> expectedResults;
  expectedResults.push_back(TUnit(V3D(-1,0,0),V3D(-0.4,0,0),4.6,4));
  expectedResults.push_back(TUnit(V3D(-0.4,0,0),V3D(0.2,0,0),5.2,3));
  expectedResults.push_back(TUnit(V3D(0.2,0,0),V3D(1,0,0),6,4));
  checkTrackIntercept(TL,expectedResults);
}

void testTrack_CubePlusInternalEdgeTouchSpheresMiss()
  /*!
    Test a track missing an object
  */
{
  std::string ObjA="60001 -60002 60003 -60004 60005 -60006 72 73";
  std::string ObjB="(-72 : -73)";
  MObj.erase(MObj.begin(),MObj.end());
  MObj[3]=Object();
  MObj[3].setObject(3,ObjA);

  MObj[4]=Object();
  MObj[4].setObject(4,ObjB);

  createSurfaces();
  
  Track TL(Geometry::V3D(-5,0,0),
	   Geometry::V3D(0,1,0));


  // CARE: This CANNOT be called twice
  TS_ASSERT_EQUALS(MObj[3].interceptSurface(TL),0);
  TS_ASSERT_EQUALS(MObj[4].interceptSurface(TL),0);

  std::vector<TUnit> expectedResults; //left empty as this should miss
  checkTrackIntercept(TL,expectedResults);
}

void testFindPointInCube()
  /*!
    Test find point in cube
  */
{
    Object A = createUnitCube();
    // initial guess in object
	Geometry::V3D pt;
	TS_ASSERT_EQUALS(A.getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0,0,0));
	// initial guess not in object, but on x-axis
	std::vector<std::string> planes;
	planes.push_back("px 10"); planes.push_back("px 11");
	planes.push_back("py -0.5"); planes.push_back("py 0.5");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
	Object B =createCuboid(planes);
    TS_ASSERT_EQUALS(B.getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(10,0,0));
	// on y axis
	planes.clear();
	planes.push_back("px -0.5"); planes.push_back("px 0.5");
	planes.push_back("py -22"); planes.push_back("py -21");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
	Object C =createCuboid(planes);
    TS_ASSERT_EQUALS(C.getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0,-21,0));
	// not on principle axis, now works using getBoundingBox
	planes.clear();
	planes.push_back("px 0.5"); planes.push_back("px 1.5");
	planes.push_back("py -22"); planes.push_back("py -21");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
	Object D =createCuboid(planes);
    TS_ASSERT_EQUALS(D.getPointInObject(pt),1);
    TS_ASSERT_DELTA(pt.X(),1.0,1e-6);
    TS_ASSERT_DELTA(pt.Y(),-21.5,1e-6);
    TS_ASSERT_DELTA(pt.Z(),0.0,1e-6);
	planes.clear();
	// Test non axis aligned (AA) case - getPointInObject works because the object is on a principle axis
	// However, if not on a principle axis then the getBoundingBox fails to find correct minima (maxima are OK)
	// This is related to use of the complement for -ve surfaces and might be avoided by only using +ve surfaces
	// for defining non-AA objects. However, BoundingBox is poor for non-AA and needs improvement if these are
	// common
	planes.push_back("p 1 0 0 -0.5"); planes.push_back("p 1 0 0 0.5");
	planes.push_back("p 0 .70710678118 .70710678118 -1.1"); planes.push_back("p 0 .70710678118 .70710678118 -0.1");
    planes.push_back("p 0 -.70710678118 .70710678118 -0.5"); planes.push_back("p 0 -.70710678118 .70710678118 0.5");
	Object E =createCuboid(planes);
    TS_ASSERT_EQUALS(E.getPointInObject(pt),1);
    TS_ASSERT_DELTA(pt.X(),0.0,1e-6);
    TS_ASSERT_DELTA(pt.Y(),-0.1414213562373,1e-6);
    TS_ASSERT_DELTA(pt.Z(),0.0,1e-6);
	planes.clear();
	// This test fails to find a point in object, as object not on a principle axis
	// and getBoundingBox does not give a useful result in this case
	planes.push_back("p 1 0 0 -0.5"); planes.push_back("p 1 0 0 0.5");
	planes.push_back("p 0  .70710678118 .70710678118 -2"); planes.push_back("p 0  .70710678118 .70710678118 -1");
    planes.push_back("p 0 -.70710678118 .70710678118 -0.5"); planes.push_back("p 0 -.70710678118 .70710678118 0.5");
	Object F =createCuboid(planes);
    TS_ASSERT_EQUALS(F.getPointInObject(pt),0);
    Object S = createSphere();
    TS_ASSERT_EQUALS(S.getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0.0,0.0,0));
}


void testSolidAngleSphere()
  /*!
    Test solid angle calculation for a sphere
  */
{
    Object A = createSphere();
    double satol=2e-2; // tolerance for solid angle

    // Solid angle at distance 8.1 from centre of sphere radius 4.1 x/y/z
    // Expected solid angle calculated values from sa=2pi(1-cos(arcsin(R/r))
    // where R is sphere radius and r is distance of observer from sphere centre
    // Intercept for track in reverse direction now worked round
    TS_ASSERT_DELTA(A.solidAngle(V3D(8.1,0,0)),0.864364,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,8.1,0)),0.864364,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,8.1)),0.864364,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,-8.1)),0.864364,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,0)),4*M_PI,satol);
    // surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(4.1,0,0)),2*M_PI,satol);
    // distant points
    TS_ASSERT_DELTA(A.solidAngle(V3D(20,0,0)),0.133442,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(200,0,0)),0.0013204,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(2000,0,0)),1.32025e-5,satol);
}

void testSolidAngleCappedCylinder()
  /*!
    Test solid angle calculation for a capped cylinder
  */
{
    Object A = createCappedCylinder();
    double satol=2e-2; // tolerance for solid angle

    // solid angle at distance 4 from capped cyl -3.2 1.2 in x, rad 3
    //
    // soild angle of circle radius 3, distance 3 is 2pi(1-cos(t)) where
    // t is atan(3/3), should be 1.840302,
    // Work round for reverse track intercept has been made in Object.cpp
    TS_ASSERT_DELTA(A.solidAngle(V3D(4.2,0,0)),1.840302,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(-7.2,0,0)),1.25663708,satol);
    // No analytic value for side on SA, using hi-res value
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,7)),0.7531,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,7,0)),0.7531,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,0)),4*M_PI,satol);
    // surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(1.2,0,0)),2*M_PI,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(-3.2,0,0)),2*M_PI,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,3,0)),2*M_PI,satol);
    // distant points
    TS_ASSERT_DELTA(A.solidAngle(V3D(20,0,0)),0.07850147,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(200,0,0)),0.000715295,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(2000,0,0)),7.08131e-6,satol);
}

void testSolidAngleCube()
  /*!
    Test solid angle calculation for a cube
  */
{
    Object A = createUnitCube();
    double satol=2e-2; // tolerance for solid angle

    // solid angle at distance 0.5 should be 4pi/6 by symmetry
    //
    TS_ASSERT_DELTA(A.solidAngle(V3D(1.0,0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(-1.0,0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,1.0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,-1.0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,1.0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,-1.0)),M_PI*2.0/3.0,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,0,0)),4*M_PI,satol);
}

void testGetBoundingBox()
/*!
  Test bounding box for a object capped cylinder
*/
{
    Object A = createCappedCylinder();
	double xmax,ymax,zmax,xmin,ymin,zmin;
	xmax=ymax=zmax=100;
	xmin=ymin=zmin=-100;
	A.getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	TS_ASSERT_DELTA(xmax,1.2,0.0001);
	TS_ASSERT_DELTA(ymax,3.0,0.0001);
	TS_ASSERT_DELTA(zmax,3.0,0.0001);
	TS_ASSERT_DELTA(xmin,-3.2,0.0001);
	TS_ASSERT_DELTA(ymin,-3.0,0.0001);
	TS_ASSERT_DELTA(zmin,-3.0,0.0001);
}
private:
 
  /// Surface type
  typedef std::map<int,Surface*> STYPE ; 
  /// Object type
  typedef std::map<int,Object> MTYPE;  

  STYPE SMap;   ///< Surface Map
  MTYPE MObj;   ///< Master object lis

  void populateMObj()
    /*!
      Populate with simple object
      the MLis component
    */
  {
    std::string ObjA="60001 -60002 60003 -60004 60005 -60006";
    std::string ObjB="-4  5  60  -61  62  -63";
    std::string ObjC="-12 13 60  -61  62  -63";
    std::string ObjD="80001 ((-80002 80003) : -80004 ) 80005 -80006";

    MObj.erase(MObj.begin(),MObj.end());
    MObj[3]=Object();
    MObj[2]=Object();
    MObj[8]=Object();
    MObj[10]=Object();
    MObj[3].setObject(3,ObjA);
    MObj[2].setObject(2,ObjB);
    MObj[8].setObject(8,ObjC);
    MObj[10].setObject(10,ObjD);
  }

  Object createCappedCylinder()
  {
    std::string C31="cx 3.0";         // cylinder x-axis radius 3
    std::string C32="px 1.2";
    std::string C33="px -3.2";

    // First create some surfaces
    std::map<int,Surface*> CylSurMap;
    CylSurMap[31]=new Cylinder();
    CylSurMap[32]=new Plane();
    CylSurMap[33]=new Plane();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21) 
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder="-31 -32 33";

    Object retVal; 
    retVal.setObject(21,ObjCapCylinder);
    retVal.populate(CylSurMap);

    return retVal;
  }

  Object createSphere()
  {
    std::string S41="so 4.1";         // Sphere at origin radius 4.1

    // First create some surfaces
    std::map<int,Surface*> SphSurMap;
    SphSurMap[41]=new Sphere();
    SphSurMap[41]->setSurface(S41);
    SphSurMap[41]->setName(41);

    // A sphere 
    std::string ObjSphere="-41" ;

    Object retVal; 
    retVal.setObject(41,ObjSphere);
    retVal.populate(SphSurMap);

    return retVal;
  }

  void clearSurfMap()
  /*!
    Clears the surface map for a new test
    or destruction.
   */
{
  STYPE::iterator mc;
  for(mc=SMap.begin();mc!=SMap.end();mc++)
    delete mc->second;
  SMap.erase(SMap.begin(),SMap.end());
  return;
}

  void createSurfaces()
    /*!
      Creates a list of surfaces for used in the objects
      and populates the MObj layers.
    */
   {
      clearSurfMap();
  
      // PLANE SURFACES:
      
      typedef std::pair<int,std::string> SCompT;
      std::vector<SCompT> SurfLine;
      SurfLine.push_back(SCompT(60001,"px -1"));
      SurfLine.push_back(SCompT(60002,"px 1"));
      SurfLine.push_back(SCompT(60003,"py -2"));
      SurfLine.push_back(SCompT(60004,"py 2"));
      SurfLine.push_back(SCompT(60005,"pz -3"));
      SurfLine.push_back(SCompT(60006,"pz 3"));
      
      SurfLine.push_back(SCompT(80001,"px 4.5"));
      SurfLine.push_back(SCompT(80002,"px 6.5"));
      
      SurfLine.push_back(SCompT(71,"so 0.8"));
      SurfLine.push_back(SCompT(72,"s -0.7 0 0 0.3"));
      SurfLine.push_back(SCompT(73,"s 0.6 0 0 0.4"));

      std::vector<SCompT>::const_iterator vc;

      // Note that the testObject now manages the "new Plane"
      Geometry::Surface* A;
      for(vc=SurfLine.begin();vc!=SurfLine.end();vc++)
	{  
	  A=Geometry::SurfaceFactory::Instance()->processLine(vc->second);
	  if (!A)
	    {
	      std::cerr<<"Failed to process line "<<vc->second<<std::endl;
	      exit(1);
	    }
	  A->setName(vc->first);
	  SMap.insert(STYPE::value_type(vc->first,A));
	}

      //   Now populate all the objects with the corresponding surface
      MTYPE::iterator mc;
      for(mc=MObj.begin();mc!=MObj.end();mc++)
	{
	  mc->second.populate(SMap);
	  //	  mc->second.write(std::cout);
	}

      return;
  
}


  Object createUnitCube()
  {
    std::string C1="px -0.5";         // cube +/-0.5
    std::string C2="px 0.5";
    std::string C3="py -0.5";
    std::string C4="py 0.5";
    std::string C5="pz -0.5";
    std::string C6="pz 0.5";

    // Create surfaces
    std::map<int,Surface*> CubeSurMap;
    CubeSurMap[1]=new Plane();
    CubeSurMap[2]=new Plane();
    CubeSurMap[3]=new Plane();
    CubeSurMap[4]=new Plane();
    CubeSurMap[5]=new Plane();
    CubeSurMap[6]=new Plane();

    CubeSurMap[1]->setSurface(C1);
    CubeSurMap[2]->setSurface(C2);
    CubeSurMap[3]->setSurface(C3);
    CubeSurMap[4]->setSurface(C4);
    CubeSurMap[5]->setSurface(C5);
    CubeSurMap[6]->setSurface(C6);
    CubeSurMap[1]->setName(1);
    CubeSurMap[2]->setName(2);
    CubeSurMap[3]->setName(3);
    CubeSurMap[4]->setName(4);
    CubeSurMap[5]->setName(5);
    CubeSurMap[6]->setName(6);

    // Cube (id 68) 
    // using surface ids:  1-6
    std::string ObjCube="1 -2 3 -4 5 -6";

    Object retVal; 
    retVal.setObject(68,ObjCube);
    retVal.populate(CubeSurMap);

    return retVal;
  }


  Object createCuboid(std::vector<std::string>& planes)
  {
    std::string C1=planes[0];
    std::string C2=planes[1];
    std::string C3=planes[2];
    std::string C4=planes[3];
    std::string C5=planes[4];
    std::string C6=planes[5];

    // Create surfaces
    std::map<int,Surface*> CubeSurMap;
    CubeSurMap[1]=new Plane();
    CubeSurMap[2]=new Plane();
    CubeSurMap[3]=new Plane();
    CubeSurMap[4]=new Plane();
    CubeSurMap[5]=new Plane();
    CubeSurMap[6]=new Plane();

    CubeSurMap[1]->setSurface(C1);
    CubeSurMap[2]->setSurface(C2);
    CubeSurMap[3]->setSurface(C3);
    CubeSurMap[4]->setSurface(C4);
    CubeSurMap[5]->setSurface(C5);
    CubeSurMap[6]->setSurface(C6);
    CubeSurMap[1]->setName(1);
    CubeSurMap[2]->setName(2);
    CubeSurMap[3]->setName(3);
    CubeSurMap[4]->setName(4);
    CubeSurMap[5]->setName(5);
    CubeSurMap[6]->setName(6);

    // Cube (id 68) 
    // using surface ids:  1-6
    std::string ObjCube="1 -2 3 -4 5 -6";

    Object retVal; 
    retVal.setObject(68,ObjCube);
    retVal.populate(CubeSurMap);

    return retVal;
  }

};

#endif //MANTID_TESTOBJECT__

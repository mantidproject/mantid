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

using namespace Mantid;
using namespace Geometry;

class testObject: public CxxTest::TestSuite
{
private:

  std::map<int,Object> MObj;

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

  Object createCappedylinder()
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
         
    // A sphere 
    std::string ObjSphere="-41" ;

    Object retVal; 
    retVal.setObject(41,ObjSphere);
    retVal.populate(SphSurMap);

    return retVal;
  }

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
    Object A = createCappedylinder();
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
    Object A = createCappedylinder();
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

};

#endif //MANTID_TESTOBJECT__

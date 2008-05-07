#ifndef MANTID_TESTOBJECT__
#define MANTID_TESTOBJECT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

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
  std::map<int,Surface*> SurMap;

 void populateMObj()
    /*!
    Populate with simple object
    the MLis component
    */
  {
    std::string C31="cx 3.0";         // cylinder x-axis radius 3
    std::string C32="px 1.2";
    std::string C33="px -3.2";
    std::string S41="so 4.0";         // Sphere at origin radius 4.0

    // First create some surfaces
    SurMap.erase(SurMap.begin(),SurMap.end());
    SurMap[31]=new Cylinder();
    SurMap[32]=new Plane();
    SurMap[33]=new Plane();
    SurMap[41]=new Sphere();

    SurMap[31]->setSurface(C31);
    SurMap[32]->setSurface(C32);
    SurMap[33]->setSurface(C33);
    SurMap[41]->setSurface(S41);

    //
    // Caped cylinder (id 21) 
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder="-31 -32 33";

    // A sphere 
    std::string ObjSphere="-41" ;

    std::string ObjA="60001 -60002 60003 -60004 60005 -60006";
    std::string ObjB="-4  5  60  -61  62  -63";
    std::string ObjC="-12 13 60  -61  62  -63";
    std::string ObjD="80001 ((-80002 80003) : -80004 ) 80005 -80006";

    MObj.erase(MObj.begin(),MObj.end());
    MObj[21]=Object(); 
    MObj[22]=Object();
    MObj[3]=Object();
    MObj[2]=Object();
    MObj[8]=Object();
    MObj[10]=Object();
    MObj[21].setObject(21,ObjCapCylinder);
    MObj[22].setObject(22,ObjSphere);
    MObj[3].setObject(3,ObjA);
    MObj[2].setObject(2,ObjB);
    MObj[8].setObject(8,ObjC);
    MObj[10].setObject(10,ObjD);

    MObj[21].populate(SurMap);
    MObj[22].populate(SurMap); 
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

  //void testComplement() 
  //  /*!
  //    Test the removal of a complement
  //    \retval -1 :: Unable to process line
  //    \retval 0 :: success
  //  */
  //{
  //  populateMObj();
  //
  //  Object A;
  //  std::string Ostr=" 4 10 0.05524655  1 -2 3 -4 5 -6  #10";
  //  A.procString(Ostr);
  //  std::cout<<"IS Complements "<<A.hasComplement()<<std::endl;
  //  Algebra AX;
  //  AX.setFunctionObjStr(A.cellStr(MObj));
  //  if (!A.procString(AX.writeMCNPX()))
  //    {
  //      std::cout<<"Failed Complement ";
  //      TS_ASSERT(1==0);
  //    }
  //  A.write(std::cout);
  //  return;
  //}
  //
  //void testMakeComplement()
  //  /*!
  //    Test the making of a given object complementary
  //   */
  //{
  //  populateMObj();
  //
  //  MObj[2].write(std::cerr);
  //  MObj[2].makeComplement();
  //  MObj[2].write(std::cerr);
  //  return;  
  //}

 
};

#endif //MANTID_TESTOBJECT__

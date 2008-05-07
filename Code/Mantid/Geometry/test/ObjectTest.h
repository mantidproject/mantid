#ifndef MANTID_TESTOBJECT__
#define MANTID_TESTOBJECT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "MantidGeometry/Object.h" 

using namespace Mantid;
using namespace Geometry;

class testObject: public CxxTest::TestSuite
{

public:


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

  // First create some surface
  SurMap.erase(SurMap.begin(),SurMap.end());
  SurMap[31]=new Cylinder();
  SurMap[32]=new Plane();
  SurMap[33]=new Plane();
  SurMap[41]=surfaceFactory::Instance()->createSurface(S41);

  SurMap[31]->setSurface(C31);
  SurMap[32]->setSurface(C32);
  SurMap[33]->setSurface(C33);
  SurMap[41]->setSurface(S41);

  //
  // Caped cylinder (id 21) with material number 27 (tungsten) 
  // density 0.065Atom/A^3
  // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
  std::string ObjCapCylinder="21 37 0.065723 -31 -32 33"

  // A sphere (vacuum)
  std::string ObjSphere="22 0 -41" 

  std::string ObjA="3 2  0.06289096  60001 -60002 60003 -60004 60005 -60006";
  std::string ObjB="2 12 0.09925325  -4  5  60  -61  62  -63";
  std::string ObjC="8 12 0.09925325  -12 13 60  -61  62  -63";
  std::string ObjD="10 2  0.06289096  80001 ((-80002 80003) : -80004 ) 80005 -80006";

  MObj.erase(MObj.begin(),MObj.end());
  MObj[21]=Object(); 
  MObj[22]=Object();
  MObj[3]=Object();
  MObj[2]=Object();
  MObj[8]=Object();
  MObj[10]=Object();
  MObj[21].setObject(ObjCapCylinder);
  MObj[22].setObject(ObjSphere);
  MObj[3].setObject(ObjA);
  MObj[2].setObject(ObjB);
  MObj[8].setObject(ObjC);
  MObj[10].setObject(ObjD);

  Mobj[21].populate(SurMap);
  Mobj[22].populate(SurMap); 
  
  return;
}


int
testCellStr()
  /*!
    Test teh CellSTr method including the 
    complementary of both #(XXX) and #Cell.
    \retval 0 :: success
  */
{
  populateMObj();
  std::cout<<"Cell == "<<MObj[2].cellStr(MObj)<<std::endl;
  Qhull A;
  std::string Ostr=" 4 10 0.05524655  -5  8  60  (-61 :  62)  -63 #3";
  A.setObject(Ostr);
  std::string Xstr = A.cellStr(MObj);
  std::cout<<"Aim: #(-6  7  60001 -61001 62001 -63001) "
	   <<"-5  8  60  (-61 : 62)  -63"<<std::endl;
  std::cout<<"Obt: "<<Xstr<<std::endl;
  // Test Two
  std::cout<<"------------------"<<std::endl;
  Ostr=" 5 1 0.05524655  18 45 #(45 (57 : 56))";
  A.setObject(Ostr);
  Xstr=A.cellStr(MObj);
  std::cout<<"Obt: "<<Xstr<<std::endl;
  std::cout<<"Aim: "<<Ostr<<std::endl;
  return;
}


int
testSetObject() 
  /*!
    Test the porcessing of a line
    \retval -1 :: Unable to process line
    \retval 0 :: success
  */
{
  Object A;
  std::string Ostr=" 4 10 0.05524655  -5  8  60  -61  62  -63 #3";
  A.setObject(Ostr);
  std::cout<<"IS Complements"<<A.hasComplement()<<std::endl;
  A.write(std::cout);
//  Algebra AX;
//  AX.setFunctionObjStr(A.cellStr());
  return;
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
  std::string Ostr=" 4 10 0.05524655  1 -2 3 -4 5 -6  #10";
  A.setObject(Ostr);
  std::cout<<"IS Complements "<<A.hasComplement()<<std::endl;
  Algebra AX;
  AX.setFunctionObjStr(A.cellStr(MObj));
  if (!A.procString(AX.writeMCNPX()))
    {
      std::cout<<"Failed Complement ";
      TS_ASSERT(1==0);
    }
  A.write(std::cout);
  return;
}

void testMakeComplement()
  /*!
    Test the making of a given object complementary
   */
{
  populateMObj();

  MObj[2].write(std::cerr);
  MObj[2].makeComplement();
  MObj[2].write(std::cerr);
  return;  
}


private:

  std::map<int,Object> MObj;
  std::map<int,Surface*> SurMap;

};

#endif

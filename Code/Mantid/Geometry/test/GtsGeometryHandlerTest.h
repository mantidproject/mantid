#ifndef MANTID_GTSGEOMETRYHANDLERTEST__
#define MANTID_GTSGEOMETRYHANDLERTEST__

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
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/GtsGeometryHandler.h"

using namespace Mantid;
using namespace Geometry;

class testGtsGeometryHandler: public CxxTest::TestSuite
{
  private:


public:
	void testCappedCylinder(){
		createComplexObject();
	}

private:
 
	void createComplexObject(){
		boost::shared_ptr<Object> complexObject=createCappedTwoSpheres();
		GtsGeometryHandler* handler = new GtsGeometryHandler(complexObject);
		createObj(complexObject);
	}

	void createObj(boost::shared_ptr<Object> complexObject){
			ObjComponent ocyl("ocyl", complexObject);
			ocyl.setPos(10,0,0);
			ocyl.setRot(Quat(90.0,V3D(0,0,1)));
			GtsGeometryHandler* compHandler=new GtsGeometryHandler(&ocyl);	
			ocyl.initDraw();
			ocyl.draw();
	}

  boost::shared_ptr<Object> createCappedTwoSpheres()
  {
   const std::string C31="so 3.0";         // outer sphere with 3 radius
   const std::string C32="so 1.0";         // inner sphere with 1 radius
   const std::string C33="px 0.0";         // plane parallel to yz plane with x=0

 //   // First create some surfaces
	std::map<int,Mantid::Geometry::Surface*> CylSurMap;
	CylSurMap[31]=new Mantid::Geometry::Sphere();
	CylSurMap[32]=new Mantid::Geometry::Sphere();
	CylSurMap[33]=new Mantid::Geometry::Plane();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

 //   // Capped cylinder (id 21) 
 //   // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder="-31 32 33";// 32";

    boost::shared_ptr<Object> retVal(new Object);
    retVal->setObject(21,ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

};

#endif //MANTID_TESTOBJECT__

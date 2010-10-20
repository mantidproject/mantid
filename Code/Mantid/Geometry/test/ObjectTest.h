#ifndef MANTID_TESTOBJECT__
#define MANTID_TESTOBJECT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <ctime>

#include <boost/shared_ptr.hpp>

#include "MantidGeometry/V3D.h" 
#include "MantidGeometry/Objects/Object.h" 
#include "MantidGeometry/Surfaces/Cylinder.h" 
#include "MantidGeometry/Surfaces/Sphere.h" 
#include "MantidGeometry/Surfaces/Plane.h" 
#include "MantidGeometry/Math/Algebra.h" 
#include "MantidGeometry/Surfaces/SurfaceFactory.h" 
#include "MantidGeometry/Objects/Track.h" 
#include "MantidGeometry/Rendering/GluGeometryHandler.h"
#include "MantidGeometry/Objects/BoundingBox.h"

using namespace Mantid;
using namespace Geometry;

typedef boost::shared_ptr<Object> Object_sptr;

class testObject: public CxxTest::TestSuite
{
private:


public:


  void testCreateUnitCube()
  {
    Object_sptr geom_obj = createUnitCube();
    
    TS_ASSERT_EQUALS(geom_obj->str(),"68 -6 5 -4 3 -2 1");

    double xmin(0.0), xmax(0.0), ymin(0.0), ymax(0.0), zmin(0.0), zmax(0.0);
    geom_obj->getBoundingBox(xmax, ymax, zmax, xmin, ymax,zmin);

  }

  void testIsOnSideCappedCylinder()
  {
	Object_sptr geom_obj = createCappedCylinder();
    //inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,0)),0); //origin
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,2.9,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-2.9,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-2.9)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,2.9)),0);
    //on the side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,3)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2,0,0)),1);

    //on the edges
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2,0,3)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2,0,3)),1);
    //out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,3.1,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-3.1,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-3.1)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,3.1)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.3,0,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.3,0,0)),0);
  }

  void testIsValidCappedCylinder()
  {
    Object_sptr geom_obj = createCappedCylinder();
    //inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,0)),1); //origin
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,2.9,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-2.9,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-2.9)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,2.9)),1);
    //on the side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,3)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2,0,0)),1);

    //on the edges
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2,0,3)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2,3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2,-3,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2,0,-3)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2,0,3)),1);
    //out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,3.1,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-3.1,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-3.1)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,3.1)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.3,0,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.3,0,0)),0);
  }

  void testIsOnSideSphere()
  {
    Object_sptr geom_obj = createSphere();
    //inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,0)),0); //origin
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,4.0,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-4.0,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-4.0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,4.0)),0);
    //on the side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,4.1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-4.1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-4.1)),1);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,4.1)),1);

    //out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,4.2,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,-4.2,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,-4.2)),0);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0,0,4.2)),0);
  }

  void testIsValidSphere()
  {
    Object_sptr geom_obj = createSphere();
    //inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,0)),1); //origin
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,4.0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-4.0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-4.0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,4.0)),1);
    //on the side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,4.1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-4.1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-4.1)),1);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,4.1)),1);

    //out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,4.2,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,-4.2,0)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,-4.2)),0);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0,0,4.2)),0);
  }

  void testCalcValidTypeSphere()
  {
    Object_sptr geom_obj = createSphere();
    //entry on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1,0,0),V3D(1,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1,0,0),V3D(-1,0,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1,0,0),V3D(1,0,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1,0,0),V3D(-1,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,-4.1,0),V3D(0,1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,-4.1,0),V3D(0,-1,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,4.1,0),V3D(0,1,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,4.1,0),V3D(0,-1,0)),1);

    //a glancing blow
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1,0,0),V3D(0,1,0)),0);
    //not quite on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1,0,0),V3D(0.5,0.5,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1,0,0),V3D(0.5,0.5,0)),-1);
  }

  void testGetBoundingBoxForSphere()
  {
    Object_sptr geom_obj = createSphere();    
    const double tolerance(1e-10);

    double xmax,ymax,zmax,xmin,ymin,zmin;
    xmax=ymax=zmax=20;
    xmin=ymin=zmin=-20;
    geom_obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
    TS_ASSERT_DELTA(xmax,4.1,tolerance);
    TS_ASSERT_DELTA(ymax,4.1,tolerance);
    TS_ASSERT_DELTA(zmax,4.1,tolerance);
    TS_ASSERT_DELTA(xmin,-4.1,tolerance);
    TS_ASSERT_DELTA(ymin,-4.1,tolerance);
    TS_ASSERT_DELTA(zmin,-4.1,tolerance);

    boost::shared_ptr<BoundingBox> bbox = geom_obj->getBoundingBox();

    TS_ASSERT_DELTA(bbox->xMax(),4.1,tolerance);
    TS_ASSERT_DELTA(bbox->yMax(),4.1,tolerance);
    TS_ASSERT_DELTA(bbox->zMax(),4.1,tolerance);
    TS_ASSERT_DELTA(bbox->xMin(),-4.1,tolerance);
    TS_ASSERT_DELTA(bbox->yMin(),-4.1,tolerance);
    TS_ASSERT_DELTA(bbox->zMin(),-4.1,tolerance);
  }

  void testCalcValidTypeCappedCylinder()
  {
    Object_sptr geom_obj = createCappedCylinder();
    //entry on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2,0,0),V3D(1,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2,0,0),V3D(-1,0,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2,0,0),V3D(1,0,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2,0,0),V3D(-1,0,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,-3,0),V3D(0,1,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,-3,0),V3D(0,-1,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,3,0),V3D(0,1,0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0,3,0),V3D(0,-1,0)),1);

    //a glancing blow
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2,0,0),V3D(0,1,0)),0);
    //not quite on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2,0,0),V3D(0.5,0.5,0)),1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2,0,0),V3D(0.5,0.5,0)),-1);
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

    Object_sptr geom_obj = Object_sptr(new Object); 
    geom_obj->setObject(41,ObjSphere);
    geom_obj->populate(SphSurMap);


    Track track(V3D(-1,1.5,1),V3D(1,0,0));

    // format = startPoint, endPoint, total distance so far, objectID
    // forward only intercepts means that start point should be track origin
    //expectedResults.push_back(TUnit(V3D(-sqrt(16-0.25)+1,1.5,1),
    expectedResults.push_back(TUnit(V3D(-1,1.5,1),
      V3D(sqrt(16-0.25)+1,1.5,1.0),sqrt(15.75)+2,geom_obj->getName()));

    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void testInterceptSurfaceSphereY()
  {
    std::vector<TUnit> expectedResults;
    Object_sptr geom_obj = createSphere();
    Track track(V3D(0,-10,0),V3D(0,1,0));

    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(0,-4.1,0),V3D(0,4.1,0),14.1,geom_obj->getName()));

    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void testInterceptSurfaceSphereX()
  {
    std::vector<TUnit> expectedResults;
    Object_sptr geom_obj = createSphere();
    Track track(V3D(-10,0,0),V3D(1,0,0));

    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(-4.1,0,0),V3D(4.1,0,0),14.1,geom_obj->getName()));
    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void testInterceptSurfaceCappedCylinderY()
  {
    std::vector<TUnit> expectedResults;
    Object_sptr geom_obj = createCappedCylinder();
    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(0,-3,0),V3D(0,3,0),13,geom_obj->getName()));

    Track track(V3D(0,-10,0),V3D(0,1,0));
    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void testInterceptSurfaceCappedCylinderX()
  {
    std::vector<TUnit> expectedResults;
    Object_sptr geom_obj = createCappedCylinder();
    Track track(V3D(-10,0,0),V3D(1,0,0));

    //format = startPoint, endPoint, total distance so far, objectID
    expectedResults.push_back(TUnit(V3D(-3.2,0,0),V3D(1.2,0,0),11.2,geom_obj->getName()));

    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void testInterceptSurfaceCappedCylinderMiss()
  {
    std::vector<TUnit> expectedResults; //left empty as there are no expected results
    Object_sptr geom_obj = createCappedCylinder();
    Track track(V3D(-10,0,0),V3D(1,1,0));

    checkTrackIntercept(geom_obj,track,expectedResults);
  }

  void checkTrackIntercept(Track& track, std::vector<TUnit>& expectedResults)
  {
    int index = 0;
    for (Track::LType::const_iterator it = track.begin(); it!=track.end();++it)
    {
      TS_ASSERT_DELTA(it->distFromStart,expectedResults[index].distFromStart,1e-6);
      TS_ASSERT_DELTA(it->distInsideObject,expectedResults[index].distInsideObject,1e-6);
      TS_ASSERT_EQUALS(it->ObjID,expectedResults[index].ObjID);
      TS_ASSERT_EQUALS(it->entryPoint,expectedResults[index].entryPoint);
      TS_ASSERT_EQUALS(it->exitPoint,expectedResults[index].exitPoint);
      ++index;
    }
    TS_ASSERT_EQUALS(index,static_cast<int>(expectedResults.size()));
  }

  void checkTrackIntercept(Object_sptr obj, Track& track, std::vector<TUnit>& expectedResults)
  {
    int unitCount = obj->interceptSurface(track);
    TS_ASSERT_EQUALS(unitCount,expectedResults.size());
    checkTrackIntercept(track,expectedResults);
  }

  void xtestTrackTwoIsolatedCubes()
    /*!
    Test a track going through an object
    */
  {
    std::string ObjA="60001 -60002 60003 -60004 60005 -60006";
    std::string ObjB="80001 -80002 60003 -60004 60005 -60006";
    
    createSurfaces(ObjA);
    Object object1=Object();
    object1.setObject(3,ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2=Object();
    object2.setObject(4,ObjB);
    object2.populate(SMap);

    Track TL(Geometry::V3D(-5,0,0),
      Geometry::V3D(1,0,0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL)!=0)
      TS_ASSERT(object2.interceptSurface(TL)!=0)

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

    createSurfaces(ObjA);
    Object object1=Object();
    object1.setObject(3,ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2=Object();
    object2.setObject(4,ObjB);
    object2.populate(SMap);

    Track TL(Geometry::V3D(-5,0,0),
      Geometry::V3D(1,0,0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL)!=0)
      TS_ASSERT(object2.interceptSurface(TL)!=0)

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

    createSurfaces(ObjA);
    Object object1=Object();
    object1.setObject(3,ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2=Object();
    object2.setObject(4,ObjB);
    object2.populate(SMap);

    Track TL(Geometry::V3D(-5,0,0),
      Geometry::V3D(1,0,0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL)!=0);
    TS_ASSERT(object2.interceptSurface(TL)!=0);

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
   
    createSurfaces(ObjA);
    Object object1=Object();
    object1.setObject(3,ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2=Object();
    object2.setObject(4,ObjB);
    object2.populate(SMap);

    Track TL(Geometry::V3D(-5,0,0),
      Geometry::V3D(1,0,0));


    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL)!=0);
    TS_ASSERT(object2.interceptSurface(TL)!=0);

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
    
    createSurfaces(ObjA);
    Object object1=Object();
    object1.setObject(3,ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2=Object();
    object2.setObject(4,ObjB);
    object2.populate(SMap);

    Track TL(Geometry::V3D(-5,0,0),
      Geometry::V3D(0,1,0));


    // CARE: This CANNOT be called twice
    TS_ASSERT_EQUALS(object1.interceptSurface(TL),0);
    TS_ASSERT_EQUALS(object2.interceptSurface(TL),0);

    std::vector<TUnit> expectedResults; //left empty as this should miss
    checkTrackIntercept(TL,expectedResults);
  }

  void testFindPointInCube()
    /*!
    Test find point in cube
    */
  {
    Object_sptr geom_obj = createUnitCube();
    // initial guess in object
    Geometry::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0,0,0));
    // initial guess not in object, but on x-axis
    std::vector<std::string> planes;
    planes.push_back("px 10"); planes.push_back("px 11");
    planes.push_back("py -0.5"); planes.push_back("py 0.5");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
    Object_sptr B =createCuboid(planes);
    TS_ASSERT_EQUALS(B->getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(10,0,0));
    // on y axis
    planes.clear();
    planes.push_back("px -0.5"); planes.push_back("px 0.5");
    planes.push_back("py -22"); planes.push_back("py -21");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
    Object_sptr C =createCuboid(planes);
    TS_ASSERT_EQUALS(C->getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0,-21,0));
    // not on principle axis, now works using getBoundingBox
    planes.clear();
    planes.push_back("px 0.5"); planes.push_back("px 1.5");
    planes.push_back("py -22"); planes.push_back("py -21");
    planes.push_back("pz -0.5"); planes.push_back("pz 0.5");
    Object_sptr D =createCuboid(planes);
    TS_ASSERT_EQUALS(D->getPointInObject(pt),1);
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
    Object_sptr E =createCuboid(planes);
    TS_ASSERT_EQUALS(E->getPointInObject(pt),1);
    TS_ASSERT_DELTA(pt.X(),0.0,1e-6);
    TS_ASSERT_DELTA(pt.Y(),-0.1414213562373,1e-6);
    TS_ASSERT_DELTA(pt.Z(),0.0,1e-6);
    planes.clear();
    // This test fails to find a point in object, as object not on a principle axis
    // and getBoundingBox does not give a useful result in this case.
    // Object is unit cube located at +-0.5 in x but centred on z=y=-1.606.. and rotated 45deg
    // to these two axes
    planes.push_back("p 1 0 0 -0.5"); planes.push_back("p 1 0 0 0.5");
    planes.push_back("p 0  .70710678118 .70710678118 -2"); planes.push_back("p 0  .70710678118 .70710678118 -1");
    planes.push_back("p 0 -.70710678118 .70710678118 -0.5"); planes.push_back("p 0 -.70710678118 .70710678118 0.5");
    Object_sptr F =createCuboid(planes);
    TS_ASSERT_EQUALS(F->getPointInObject(pt),0);
    // Test use of defineBoundingBox to explictly set the bounding box, when the automatic method fails
    F->defineBoundingBox(0.5,-1/(2.0*sqrt(2.0)),-1.0/(2.0*sqrt(2.0)),
      -0.5,-sqrt(2.0)-1.0/(2.0*sqrt(2.0)),-sqrt(2.0)-1.0/(2.0*sqrt(2.0)));
    TS_ASSERT_EQUALS(F->getPointInObject(pt),1);
    Object_sptr S = createSphere();
    TS_ASSERT_EQUALS(S->getPointInObject(pt),1);
    TS_ASSERT_EQUALS(pt,V3D(0.0,0.0,0));
  }


  void testSolidAngleSphere()
    /*!
    Test solid angle calculation for a sphere
    */
  {
    Object_sptr geom_obj = createSphere();
    double satol=2e-2; // tolerance for solid angle

    // Solid angle at distance 8.1 from centre of sphere radius 4.1 x/y/z
    // Expected solid angle calculated values from sa=2pi(1-cos(arcsin(R/r))
    // where R is sphere radius and r is distance of observer from sphere centre
    // Intercept for track in reverse direction now worked round
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(8.1,0,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(0,8.1,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(0,0,8.1)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(0,0,-8.1)),0.864364,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(0,0,0)),4*M_PI,satol);
    // surface point
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(4.1,0,0)),2*M_PI,satol);
    // distant points
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(20,0,0)),0.133442,satol);
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(200,0,0)),0.0013204,satol);
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(2000,0,0)),1.32025e-5,satol);
    //
    // test solidAngle interface, which will be main method to solid angle
    //
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(8.1,0,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0,8.1,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0,0,8.1)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0,0,-8.1)),0.864364,satol);
    //
  }

  void testSolidAngleCappedCylinder()
    /*!
    Test solid angle calculation for a capped cylinder
    */
  {
    Object_sptr geom_obj = createSmallCappedCylinder();
    // Want to test triangulation so setup a geometry handler
    boost::shared_ptr<GluGeometryHandler> h = boost::shared_ptr<GluGeometryHandler>(new GluGeometryHandler(geom_obj.get()));
    h->setCylinder(V3D(-1.0,0.0,0.0), V3D(1., 0.0, 0.0), 0.005, 0.003);
    geom_obj->setGeometryHandler(h);

    double satol(1e-4); // tolerance for solid angle

    // solid angle at point -0.5 from capped cyl -1.0 -0.997 in x, rad 0.005 - approx WISH cylinder
    //
    // soild angle of circle radius 3, distance 3 is 2pi(1-cos(t)) where
    // t is atan(3/3), should be 0.000317939
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.5, 0.0, 0.0)), 0.000317939, satol);
    // Other end
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.497, 0.0, 0.0)), 0.000317939, satol);

    // No analytic value for side on SA, using hi-res value
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, 0.1)), 8.03225e-05, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0.1, 0)), 8.03225e-05, satol);

    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.999, 0.0, 0.0)),4*M_PI,satol);

    // surface points
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.0, 0.0, 0.0)),2*M_PI,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.997, 0.0, 0.0)),2*M_PI,satol);

  }

  void testSolidAngleCubeTriangles()
    /*!
    Test solid angle calculation for a cube using triangles
    - test for using Open Cascade surface triangulation for all solid angles.
    */
  {
    Object_sptr geom_obj = createUnitCube();
    double satol=1e-3; // tolerance for solid angle

    // solid angle at distance 0.5 should be 4pi/6 by symmetry
    //
    // tests for Triangulated cube
    //
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(1.0,0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.0,0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,1.0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,-1.0,0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,1.0)),M_PI*2.0/3.0,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,-1.0)),M_PI*2.0/3.0,satol);

    if(timeTest)
    {
      // block to test time of solid angle methods
      // change false to true to include
      double saRay,saTri;
      V3D observer(1.0,0,0);
      int iter=4000;
      int starttime=clock();
      for (int i=0;i<iter;i++)
        saTri=geom_obj->triangleSolidAngle(observer);
      int endtime=clock();
      std::cout << std::endl << "Cube tri time=" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
      iter=50;
      starttime=clock();
      for (int i=0;i<iter;i++)
        saRay=geom_obj->rayTraceSolidAngle(observer);
      endtime=clock();
      std::cout << "Cube ray time=" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
    }

  }

  void testGetBoundingBoxForCylinder()
    /*!
    Test bounding box for a object capped cylinder
    */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax,ymax,zmax,xmin,ymin,zmin;
    xmax=ymax=zmax=100;
    xmin=ymin=zmin=-100;
    geom_obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
    TS_ASSERT_DELTA(xmax,1.2,0.0001);
    TS_ASSERT_DELTA(ymax,3.0,0.0001);
    TS_ASSERT_DELTA(zmax,3.0,0.0001);
    TS_ASSERT_DELTA(xmin,-3.2,0.0001);
    TS_ASSERT_DELTA(ymin,-3.0,0.0001);
    TS_ASSERT_DELTA(zmin,-3.0,0.0001);
  }

  void testdefineBoundingBox()
    /*!
    Test use of defineBoundingBox
    */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax,ymax,zmax,xmin,ymin,zmin;
    xmax=1.2;ymax=3.0;zmax=3.0;
    xmin=-3.2;ymin=-3.0;zmin=-3.0;

    TS_ASSERT_THROWS_NOTHING(geom_obj->defineBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin));
    
    boost::shared_ptr<BoundingBox> boundBox = geom_obj->getBoundingBox();

    TS_ASSERT_EQUALS(boundBox->xMax(),1.2);
    TS_ASSERT_EQUALS(boundBox->yMax(),3.0);
    TS_ASSERT_EQUALS(boundBox->zMax(),3.0);
    TS_ASSERT_EQUALS(boundBox->xMin(),-3.2);
    TS_ASSERT_EQUALS(boundBox->yMin(),-3.0);
    TS_ASSERT_EQUALS(boundBox->zMin(),-3.0);

    //Inconsistent bounding box
    xmax=1.2;xmin=3.0;
    TS_ASSERT_THROWS(geom_obj->defineBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin),std::invalid_argument);

  }
  void testSurfaceTriangulation()
    /*!
    Test triangle solid angle calc
    */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax,ymax,zmax,xmin,ymin,zmin;
    xmax=20;ymax=20.0;zmax=20.0;
    xmin=-20.0;ymin=-20.0;zmin=-20.0;
    geom_obj->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
    double saTri,saRay;
    V3D observer(4.2,0,0);
    
    double satol=1e-3; // typical result tolerance

    if(timeTest)
    {
      // block to test time of solid angle methods
      // change false to true to include
      int iter=4000;
      int starttime=clock();
      for (int i=0;i<iter;i++)
        saTri=geom_obj->triangleSolidAngle(observer);
      int endtime=clock();
      std::cout << std::endl << "Cyl tri time=" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
      iter=50;
      starttime=clock();
      for (int i=0;i<iter;i++)
        saRay=geom_obj->rayTraceSolidAngle(observer);
      endtime=clock();
      std::cout << "Cyl ray time=" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
    }

    saTri=geom_obj->triangleSolidAngle(observer);
    saRay=geom_obj->rayTraceSolidAngle(observer);
    TS_ASSERT_DELTA(saTri,1.840302,0.001);
    TS_ASSERT_DELTA(saRay,1.840302,0.01);
    
    observer=V3D(-7.2,0,0);
    saTri=geom_obj->triangleSolidAngle(observer);
    saRay=geom_obj->rayTraceSolidAngle(observer);
    
    TS_ASSERT_DELTA(saTri,1.25663708,0.001);
    TS_ASSERT_DELTA(saRay,1.25663708,0.001);

    // No analytic value for side on SA, using hi-res value
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,7)),0.7531,0.753*satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,7,0)),0.7531,0.753*satol);

    saTri=geom_obj->triangleSolidAngle(V3D(20,0,0));
    TS_ASSERT_DELTA(saTri,0.07850147,satol*0.0785);
    saTri=geom_obj->triangleSolidAngle(V3D(200,0,0));
    TS_ASSERT_DELTA(saTri,0.000715295,satol*0.000715);
    saTri=geom_obj->triangleSolidAngle(V3D(2000,0,0));
    TS_ASSERT_DELTA(saTri,7.08131e-6,satol*7.08e-6);
    
  }
  void testSolidAngleSphereTri()
    /*!
    Test solid angle calculation for a sphere from triangulation
    */
  {
    Object_sptr geom_obj = createSphere();
    double satol=1e-3; // tolerance for solid angle

    // Solid angle at distance 8.1 from centre of sphere radius 4.1 x/y/z
    // Expected solid angle calculated values from sa=2pi(1-cos(arcsin(R/r))
    // where R is sphere radius and r is distance of observer from sphere centre
    // Intercept for track in reverse direction now worked round
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(8.1,0,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,8.1,0)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,8.1)),0.864364,satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,-8.1)),0.864364,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0,0,0)),4*M_PI,satol);
    // surface point
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(4.1,0,0)),2*M_PI,satol);
    // distant points
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(20,0,0)),0.133442,satol*0.133);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(200,0,0)),0.0013204,satol*0.00132);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(2000,0,0)),1.32025e-5,satol*1.32e-5);

    if(timeTest)
    {
      // block to test time of solid angle methods
      // change false to true to include
      double saTri,saRay;
      int iter=400;
      V3D observer(8.1,0,0);
      int starttime=clock();
      for (int i=0;i<iter;i++)
        saTri=geom_obj->triangleSolidAngle(observer);
      int endtime=clock();
      std::cout << std::endl << "Sphere tri time =" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
      iter=40;
      starttime=clock();
      for (int i=0;i<iter;i++)
        saRay=geom_obj->rayTraceSolidAngle(observer);
      endtime=clock();
      std::cout << "Sphere ray time =" << (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) << std::endl;
    }

  }

private:

  /// Surface type
  typedef std::map<int,Surface*> STYPE ; 

  /// set timeTest true to get time comparisons of soild angle methods
  const static bool timeTest=false;
  
  STYPE SMap;   ///< Surface Map

  Object_sptr createCappedCylinder()
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

	Object_sptr retVal = Object_sptr(new Object); 
    retVal->setObject(21,ObjCapCylinder);
    retVal->populate(CylSurMap);

	TS_ASSERT(retVal.get())

    return retVal;
  }
  
  // This creates a cylinder to test the solid angle that is more realistic in size
  // for a detector cylinder
  Object_sptr createSmallCappedCylinder()
  {
    std::string C31="cx 0.005";         // cylinder x-axis radius 0.005 and height 0.003
    std::string C32="px -0.997";
    std::string C33="px -1.0";

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

    Object_sptr retVal = Object_sptr(new Object); 
    retVal->setObject(21,ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

  Object_sptr createSphere()
  {
    std::string S41="so 4.1";         // Sphere at origin radius 4.1

    // First create some surfaces
    std::map<int,Surface*> SphSurMap;
    SphSurMap[41]=new Sphere();
    SphSurMap[41]->setSurface(S41);
    SphSurMap[41]->setName(41);

    // A sphere 
    std::string ObjSphere="-41" ;

    Object_sptr retVal = Object_sptr(new Object); 
    retVal->setObject(41,ObjSphere);
    retVal->populate(SphSurMap);

    return retVal;
  }

  void clearSurfMap()
    /*!
    Clears the surface map for a new test
    or destruction.
    */
  {
    SMap.clear();
    return;
  }

  void createSurfaces(const std::string& desired)
    /*!
    Creates a list of surfaces for used in the objects
    and populates the MObj layers.
    */
  {
    clearSurfMap();

    // PLANE SURFACES:

    typedef std::pair<int,std::string> SCompT;
    std::vector<SCompT> SurfLine;
    if (desired.find("60001") != std::string::npos)
      SurfLine.push_back(SCompT(60001,"px -1"));
    if (desired.find("60002") != std::string::npos)
      SurfLine.push_back(SCompT(60002,"px 1"));
    if (desired.find("60003") != std::string::npos)
      SurfLine.push_back(SCompT(60003,"py -2"));
    if (desired.find("60004") != std::string::npos)
      SurfLine.push_back(SCompT(60004,"py 2"));
    if (desired.find("60005") != std::string::npos)
      SurfLine.push_back(SCompT(60005,"pz -3"));
    if (desired.find("60006") != std::string::npos)
      SurfLine.push_back(SCompT(60006,"pz 3"));

    if (desired.find("80001") != std::string::npos)
      SurfLine.push_back(SCompT(80001,"px 4.5"));
    if (desired.find("80002") != std::string::npos)
      SurfLine.push_back(SCompT(80002,"px 6.5"));

    if (desired.find("71") != std::string::npos)
      SurfLine.push_back(SCompT(71,"so 0.8"));
    if (desired.find("72") != std::string::npos)
      SurfLine.push_back(SCompT(72,"s -0.7 0 0 0.3"));
    if (desired.find("73") != std::string::npos)
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

    return;
  }


  Object_sptr createUnitCube()
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

    Object_sptr retVal = Object_sptr(new Object); 
    retVal->setObject(68,ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }


  Object_sptr createCuboid(std::vector<std::string>& planes)
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

    Object_sptr retVal = Object_sptr(new Object); 
    retVal->setObject(68,ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }

};

#endif //MANTID_TESTOBJECT__

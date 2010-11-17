#ifndef RECTANGULAR_DETECTOR_TEST_H
#define RECTANGULAR_DETECTOR_TEST_H

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Quat.h"
#include "ComponentCreationHelpers.hh"
#include "MantidGeometry/Objects/ShapeFactory.h"


using namespace Mantid;
using namespace Mantid::Geometry;

class RectangularDetectorTest : public CxxTest::TestSuite
{
public:
	void testEmptyConstructor()
  {
	  RectangularDetector q;
    TS_ASSERT_EQUALS(q.nelements(), 0);
    TS_ASSERT_THROWS(q[0], std::runtime_error);

    TS_ASSERT_EQUALS(q.getName(), "");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameValueConstructor()
  {
    RectangularDetector q("Name");
    TS_ASSERT_EQUALS(q.nelements(), 0);
    TS_ASSERT_THROWS(q[0], std::runtime_error);

    TS_ASSERT_EQUALS(q.getName(), "Name");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameParentValueConstructor()
  {
    CompAssembly *parent = new CompAssembly("Parent");
    parent->setPos(1,2,3);

    //name and parent
    RectangularDetector *q = new RectangularDetector("Child", parent);
    q->setPos(1,1,1);

    TS_ASSERT_EQUALS(q->getName(), "Child");
    TS_ASSERT_EQUALS(q->nelements(), 0);
    TS_ASSERT_THROWS((*q)[0], std::runtime_error);
    //check the parent
    TS_ASSERT(q->getParent());
    TS_ASSERT_EQUALS(q->getParent()->getName(), parent->getName());

    //1,1,1 is added to (1,2,3)
    TS_ASSERT_EQUALS(q->getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(q->getRelativeRot(), Quat(1, 0, 0, 0));

    // Now test the parametrized version of that
    ParameterMap_sptr pmap( new ParameterMap() );
    RectangularDetector *pq = new RectangularDetector(q, pmap.get());
    TS_ASSERT_EQUALS(pq->getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(pq->getRelativeRot(), Quat(1, 0, 0, 0));

    delete parent;
  }

  void testFullConstructor()
  {

    // --- Create a cuboid shape for your pixels ----
    double szX=0.5;
    double szY=szX;
    double szZ=szX;
    std::ostringstream xmlShapeStream;
    xmlShapeStream
        << " <cuboid id=\"detector-shape\"> "
        << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
        << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> "
        << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
        << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> "
        << "</cuboid>";

    std::string xmlCuboidShape(xmlShapeStream.str());
    Geometry::ShapeFactory shapeCreator;
    boost::shared_ptr<Geometry::Object> cuboidShape = shapeCreator.createShape(xmlCuboidShape);

    RectangularDetector *det = new RectangularDetector("MyRectangle");
    det->setPos(1000., 2000., 3000.);

    //Initialize with these parameters
    det->initialize(cuboidShape, 100, -50.0, 1.0,   200, -100.0, 1.0, 1000000, true, 1000 );

    do_test_on(det);

    // --- Now make a parametrized version ----
    ParameterMap_sptr pmap( new ParameterMap() );
    RectangularDetector *parDet = new RectangularDetector(det, pmap.get());

    do_test_on(parDet);

    delete det;
    delete parDet;
  }


  /** Test on a rectangular detector that will be
   * repeated on an un-moved parametrized version.
   */
  void do_test_on(RectangularDetector *det)
  {
    TS_ASSERT_EQUALS( det->xpixels(), 100);
    TS_ASSERT_EQUALS( det->xstart(), -50);
    TS_ASSERT_EQUALS( det->xstep(), 1.0);
    TS_ASSERT_EQUALS( det->xsize(), 100.0);
    TS_ASSERT_EQUALS( det->ypixels(), 200);
    TS_ASSERT_EQUALS( det->ystart(), -100);
    TS_ASSERT_EQUALS( det->ystep(), 1.0);
    TS_ASSERT_EQUALS( det->ysize(), 200.0);

    //Go out of bounds
    TS_ASSERT_THROWS(det->getAtXY(-1,0), std::runtime_error);
    TS_ASSERT_THROWS(det->getAtXY(0,-1), std::runtime_error);
    TS_ASSERT_THROWS(det->getAtXY(100,0), std::runtime_error);
    TS_ASSERT_THROWS(det->getAtXY(0,205), std::runtime_error);

    //Check some ids
    TS_ASSERT_EQUALS(det->getAtXY(0,0)->getID() - 1000000, 0);
    TS_ASSERT_EQUALS(det->getAtXY(0,12)->getID() - 1000000, 12);
    TS_ASSERT_EQUALS(det->getAtXY(0,112)->getID() - 1000000, 112);
    TS_ASSERT_EQUALS(det->getAtXY(1,12)->getID() - 1000000, 1012);

    //Check some positions
    TS_ASSERT_EQUALS(det->getAtXY(0,0)->getPos(), V3D( 1000-50., 2000-100., 3000.) );
    TS_ASSERT_EQUALS(det->getAtXY(1,0)->getPos(), V3D( 1000-50.+1., 2000-100., 3000.) );
    TS_ASSERT_EQUALS(det->getAtXY(1,1)->getPos(), V3D( 1000-50.+1., 2000-100.+1., 3000.) );

    //Name
    TS_ASSERT_EQUALS(det->getAtXY(1,2)->getName(), "MyRectangle(1,2)");

    BoundingBox box;
    det->getBoundingBox(box);
    TS_ASSERT_DELTA(box.xMin(), 949.5, 1e-08);
    TS_ASSERT_DELTA(box.yMin(), 1899.5, 1e-08);
    TS_ASSERT_DELTA(box.zMin(), 2999.5, 1e-08);
    TS_ASSERT_DELTA(box.xMax(), 1049.5, 1e-08);
    TS_ASSERT_DELTA(box.yMax(), 2099.5, 1e-08);
    TS_ASSERT_DELTA(box.zMax(), 3000.5, 1e-08);

    // Pull out a component and check that
    boost::shared_ptr<Detector> pixelDet = det->getAtXY(1,2);
    box = BoundingBox();
    pixelDet->getBoundingBox(box);
    TS_ASSERT_DELTA(box.xMin(), 950.5, 1e-08);
    TS_ASSERT_DELTA(box.yMin(), 1901.5, 1e-08);
    TS_ASSERT_DELTA(box.zMin(), 2999.5, 1e-08);
    TS_ASSERT_DELTA(box.xMax(), 951.5, 1e-08);
    TS_ASSERT_DELTA(box.yMax(), 1902.5, 1e-08);
    TS_ASSERT_DELTA(box.zMax(), 3000.5, 1e-08);
  }


};

#endif

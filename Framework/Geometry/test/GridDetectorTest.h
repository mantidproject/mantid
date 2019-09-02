// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GRID_DETECTOR_TEST_H
#define GRID_DETECTOR_TEST_H

#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

class GridDetectorTest : public CxxTest::TestSuite {
public:
  void testNameValueConstructor() {
    GridDetector q("Name");
    TS_ASSERT_EQUALS(q.nelements(), 0);
    TS_ASSERT_THROWS(q[0], const std::runtime_error &);

    TS_ASSERT_EQUALS(q.getName(), "Name");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    // as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameParentValueConstructor() {
    CompAssembly *parent = new CompAssembly("Parent");
    parent->setPos(1, 2, 3);

    // name and parent
    GridDetector *q = new GridDetector("Child", parent);
    q->setPos(1, 1, 1);

    TS_ASSERT_EQUALS(q->getName(), "Child");
    TS_ASSERT_EQUALS(q->nelements(), 0);
    TS_ASSERT_THROWS((*q)[0], const std::runtime_error &);
    // check the parent
    TS_ASSERT(q->getParent());
    TS_ASSERT_EQUALS(q->getParent()->getName(), parent->getName());

    // 1,1,1 is added to (1,2,3)
    TS_ASSERT_EQUALS(q->getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(q->getRelativeRot(), Quat(1, 0, 0, 0));

    // Now test the parametrized version of that
    ParameterMap_sptr pmap(new ParameterMap());
    GridDetector pq(q, pmap.get());
    TS_ASSERT_EQUALS(pq.getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));

    delete parent;
  }

  void testCorrectNameComparison() {
    // Test allowed names
    TS_ASSERT(GridDetector::compareName("GridDetector"));
    TS_ASSERT(GridDetector::compareName("gridDetector"));
    TS_ASSERT(GridDetector::compareName("griddetector"));
    TS_ASSERT(GridDetector::compareName("grid_detector"));

    // Test fail on incorrect names
    TS_ASSERT(!GridDetector::compareName("Grid Detector"));
    TS_ASSERT(!GridDetector::compareName("Grid"));
    TS_ASSERT(!GridDetector::compareName("Detector"));
  }

  void testConstructorThrowsWithInvalidFillOrderString() {
    GridDetector *det = new GridDetector("MyGrid");
    auto cuboidShape = ComponentCreationHelper::createCuboid(0.5);
    // Initialize with these parameters
    TS_ASSERT_THROWS(det->initialize(cuboidShape, 100, -50.0, 1.0, 200, -100.0,
                                     1.0, 300, -20, 1.0, 1000000, "abc", 1000,
                                     1),
                     const std::invalid_argument &);
  }

  void testFullConstructor() {
    auto cuboidShape = ComponentCreationHelper::createCuboid(0.5);

    GridDetector *det = new GridDetector("MyGrid");
    det->setPos(0, 0, 0);

    // Initialize with these parameters
    det->initialize(cuboidShape, 5, -2.5, 1.0, 7, -3.5, 1.0, 3, -1.5, 1.0,
                    1000000, "zyx", 3, 1);

    do_test_on(det);

    // --- Now make a parametrized version ----
    ParameterMap_sptr pmap(new ParameterMap());
    GridDetector *parDet = new GridDetector(det, pmap.get());

    do_test_on(parDet);

    delete det;
    delete parDet;
  }

  /** Create a parametrized GridDetector with a parameter that
   * resizes it.
   */
  void testResizingParameter() {
    auto cuboidShape = ComponentCreationHelper::createCuboid(0.5);

    GridDetector *det = new GridDetector("MyGrid");
    det->setPos(1, 2, 1);
    det->initialize(cuboidShape, 5, -2.5, 1.0, 7, -3.5, 1.0, 3, -1.5, 1.0,
                    1000000, "zyx", 3, 1);

    // --- Now make a parametrized version ----
    ParameterMap_sptr pmap(new ParameterMap());
    GridDetector *parDet = new GridDetector(det, pmap.get());
    pmap->addDouble(det, "scalex", 3.0);
    pmap->addDouble(det, "scaley", 5.0);
    pmap->addDouble(det, "scalez", 2.0);

    // Sizes and steps are scaled by these factors
    TS_ASSERT_DELTA(parDet->xstep(), 3.0, 1e-5);
    TS_ASSERT_DELTA(parDet->ystep(), 5.0, 1e-5);
    TS_ASSERT_DELTA(parDet->zstep(), 2.0, 1e-5);
    TS_ASSERT_DELTA(parDet->xstart(), -7.5, 1e-5);
    TS_ASSERT_DELTA(parDet->ystart(), -17.5, 1e-5);
    TS_ASSERT_DELTA(parDet->zstart(), -3.0, 1e-5);
    TS_ASSERT_DELTA(parDet->xsize(), 15.0, 1e-5);
    TS_ASSERT_DELTA(parDet->ysize(), 35.0, 1e-5);
    TS_ASSERT_DELTA(parDet->zsize(), 6.0, 1e-5);

    auto pos = parDet->getRelativePosAtXYZ(1, 1, 1);
    TS_ASSERT_EQUALS(pos, V3D(-4.5, -12.5, -1.0));

    // Check some positions
    pos = parDet->getAtXYZ(0, 0, 0)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-6.5, -15.5, -2.0));
    pos = parDet->getAtXYZ(1, 0, 0)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-3.5, -15.5, -2.0));
    pos = parDet->getAtXYZ(1, 1, 2)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-3.5, -10.5, 2.0));

    delete det;
    delete parDet;
  }

private:
  /** Test on a grid detector that will be
   * repeated on an un-moved parametrized version.
   */
  void do_test_on(GridDetector *det) {
    do_test_basics(det);
    do_test_bounds(det);
    do_test_ids(det);
    do_test_positions(det);

    // Name
    TS_ASSERT_EQUALS(det->getAtXYZ(1, 2, 0)->getName(), "MyGrid(1,2,0)");
    TS_ASSERT_EQUALS(det->getChild(1)->getName(), "MyGrid(z=1)");
    auto layer = boost::dynamic_pointer_cast<ICompAssembly>(det->getChild(2));
    TS_ASSERT_EQUALS(layer->getChild(1)->getName(), "MyGrid(z=2,x=1)");

    // Bounding box takes into account size of cuboid centered around zero.
    BoundingBox box;
    det->getBoundingBox(box);
    TS_ASSERT_DELTA(box.xMin(), -3.0, 1e-08);
    TS_ASSERT_DELTA(box.yMin(), -4.0, 1e-08);
    TS_ASSERT_DELTA(box.zMin(), -2.0, 1e-08);
    TS_ASSERT_DELTA(box.xMax(), 2.0, 1e-08);
    TS_ASSERT_DELTA(box.yMax(), 3.0, 1e-08);
    TS_ASSERT_DELTA(box.zMax(), 1.0, 1e-08);

    // Pull out a component and check that
    // position of det (-1.5, -1.5, -0.5) and size 0.5
    auto pixelDet = det->getAtXYZ(1, 2, 1);
    box = BoundingBox();
    pixelDet->getBoundingBox(box);
    TS_ASSERT_DELTA(box.xMin(), -2.0, 1e-08);
    TS_ASSERT_DELTA(box.yMin(), -2.0, 1e-08);
    TS_ASSERT_DELTA(box.zMin(), -1.0, 1e-08);
    TS_ASSERT_DELTA(box.xMax(), -1.0, 1e-08);
    TS_ASSERT_DELTA(box.yMax(), -1.0, 1e-08);
    TS_ASSERT_DELTA(box.zMax(), 0.0, 1e-08);
  }

  void do_test_basics(GridDetector *det) {
    TS_ASSERT_EQUALS(det->xpixels(), 5);
    TS_ASSERT_EQUALS(det->xstart(), -2.5);
    TS_ASSERT_EQUALS(det->xstep(), 1.0);
    TS_ASSERT_EQUALS(det->xsize(), 5.0);
    TS_ASSERT_EQUALS(det->ypixels(), 7.0);
    TS_ASSERT_EQUALS(det->ystart(), -3.5);
    TS_ASSERT_EQUALS(det->ystep(), 1.0);
    TS_ASSERT_EQUALS(det->ysize(), 7.0);
    TS_ASSERT_EQUALS(det->zpixels(), 3.0);
    TS_ASSERT_EQUALS(det->zstart(), -1.5);
    TS_ASSERT_EQUALS(det->zstep(), 1.0);
    TS_ASSERT_EQUALS(det->zsize(), 3.0);
  }

  void do_test_bounds(GridDetector *det) {
    // Go out of bounds
    TS_ASSERT_THROWS(det->getAtXYZ(-1, 0, 0), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXYZ(0, -1, 0), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXYZ(100, 0, 0), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXYZ(0, 205, 0), const std::runtime_error &);
  }

  void do_test_ids(GridDetector *det) {
    // Check some ids
    TS_ASSERT_EQUALS(det->getAtXYZ(0, 0, 1)->getID() - 1000000, 1);
    TS_ASSERT_EQUALS(det->getAtXYZ(0, 1, 0)->getID() - 1000000, 3);
    TS_ASSERT_EQUALS(det->getAtXYZ(1, 0, 0)->getID() - 1000000, 21);
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(0, 0, 1), 1000001);
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(0, 1, 0), 1000003);
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(1, 0, 0), 1000021);
  }

  void do_test_positions(GridDetector *det) {
    std::tuple<int, int, int> xyz;
    int x;
    int y;
    int z;

    TS_ASSERT_THROWS_NOTHING(xyz = det->getXYZForDetectorID(1000000));
    x = std::get<0>(xyz);
    y = std::get<1>(xyz);
    z = std::get<2>(xyz);
    TS_ASSERT_EQUALS(x, 0);
    TS_ASSERT_EQUALS(y, 0);
    TS_ASSERT_EQUALS(z, 0);

    TS_ASSERT_THROWS_NOTHING(xyz = det->getXYZForDetectorID(1000000 + 22));
    x = std::get<0>(xyz);
    y = std::get<1>(xyz);
    z = std::get<2>(xyz);
    TS_ASSERT_EQUALS(x, 1);
    TS_ASSERT_EQUALS(y, 0);
    TS_ASSERT_EQUALS(z, 1);

    // Check some positions
    auto pos = det->getAtXYZ(0, 0, 0)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-2.5, -3.5, -1.5));
    pos = det->getAtXYZ(1, 0, 0)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-1.5, -3.5, -1.5));
    pos = det->getAtXYZ(1, 1, 0)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-1.5, -2.5, -1.5));
    pos = det->getAtXYZ(2, 5, 2)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(-0.5, 1.5, 0.5));
  }
};

#endif

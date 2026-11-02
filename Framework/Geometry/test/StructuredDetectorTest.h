// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

class StructuredDetectorTest : public CxxTest::TestSuite {
public:
  void test_getComponentByName_of_its_own_name_does_not_take_ownership() {
    auto det = makeOnePixelDetector();
    {
      auto const self = det->getComponentByName("structured");
      TS_ASSERT_EQUALS(self.get(), det.get());
    }
    // The returned pointer has gone; the detector must still be alive.
    TS_ASSERT_EQUALS(det->getName(), "structured");
  }

  void test_getBoundingBox_of_an_unparametrized_detector() {
    auto det = makeOnePixelDetector();
    BoundingBox box;
    TS_ASSERT_THROWS_NOTHING(det->getBoundingBox(box));
    TS_ASSERT_DELTA(box.xMin(), 0.0, 1e-9);
    TS_ASSERT_DELTA(box.xMax(), 1.0, 1e-9);
    TS_ASSERT_DELTA(box.yMin(), 0.0, 1e-9);
    TS_ASSERT_DELTA(box.yMax(), 1.0, 1e-9);
  }

  void testEmptyConstructor() {
    StructuredDetector q;
    TS_ASSERT_EQUALS(q.nelements(), 0);

    // test no vertices
    TS_ASSERT_EQUALS(q.getXValues().size(), 0);
    TS_ASSERT_EQUALS(q.getYValues().size(), 0);

    // test no colours
    TS_ASSERT_EQUALS(q.getR().size(), 0);
    TS_ASSERT_EQUALS(q.getG().size(), 0);
    TS_ASSERT_EQUALS(q.getB().size(), 0);
    TS_ASSERT_THROWS(q[0], const std::runtime_error &);

    TS_ASSERT_EQUALS(q.getName(), "");
    TS_ASSERT(!q.getParent());
    TS_ASSERT_EQUALS(q.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(q.getRelativeRot(), Quat(1, 0, 0, 0));
    // as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(q.getRelativePos(), q.getPos());
  }

  void testNameValueConstructor() {
    StructuredDetector q("Name");
    TS_ASSERT_EQUALS(q.nelements(), 0);

    // test no vertices
    TS_ASSERT_EQUALS(q.getXValues().size(), 0);
    TS_ASSERT_EQUALS(q.getYValues().size(), 0);

    // test no colours
    TS_ASSERT_EQUALS(q.getR().size(), 0);
    TS_ASSERT_EQUALS(q.getG().size(), 0);
    TS_ASSERT_EQUALS(q.getB().size(), 0);

    TS_ASSERT_THROWS(q[0], const std::runtime_error &);
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
    StructuredDetector *q = new StructuredDetector("Child", parent);
    q->setPos(1, 1, 1);

    TS_ASSERT_EQUALS(q->getName(), "Child");
    TS_ASSERT_EQUALS(q->nelements(), 0);

    // test no vertices
    TS_ASSERT_EQUALS(q->getXValues().size(), 0);
    TS_ASSERT_EQUALS(q->getYValues().size(), 0);

    // test no colours
    TS_ASSERT_EQUALS(q->getR().size(), 0);
    TS_ASSERT_EQUALS(q->getG().size(), 0);
    TS_ASSERT_EQUALS(q->getB().size(), 0);

    TS_ASSERT_THROWS((*q)[0], const std::runtime_error &);
    // check the parent
    TS_ASSERT(q->getParent());
    TS_ASSERT_EQUALS(q->getParent()->getName(), parent->getName());

    // 1,1,1 is added to (1,2,3)
    TS_ASSERT_EQUALS(q->getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(q->getRelativeRot(), Quat(1, 0, 0, 0));

    // Now test the parametrized version of that
    ParameterMap_sptr pmap(new ParameterMap());
    StructuredDetector pq(q, pmap.get());
    TS_ASSERT_EQUALS(pq.getPos(), V3D(2, 3, 4));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));

    delete parent;
  }

  void testCorrectNameComparison() {
    // Test allowed names
    TS_ASSERT(StructuredDetector::compareName("StructuredDetector"));
    TS_ASSERT(StructuredDetector::compareName("structuredDetector"));
    TS_ASSERT(StructuredDetector::compareName("structureddetector"));
    TS_ASSERT(StructuredDetector::compareName("structured_detector"));

    // Test fail on incorrect names
    TS_ASSERT(!StructuredDetector::compareName("Structured Detector"));
    TS_ASSERT(!StructuredDetector::compareName("Structured"));
    TS_ASSERT(!StructuredDetector::compareName("Detector"));
  }

  void testFullConstructor() {
    auto cuboidShape = ComponentCreationHelper::createCuboid(0.5);

    StructuredDetector *det = new StructuredDetector("MyStructuredDetector");
    det->setPos(1000., 2000., 3000.);

    std::vector<double> x{0, 1, 2, 0, 1, 2, 0, 1, 2};
    std::vector<double> y{0, 0, 0, 1, 1, 1, 2, 2, 2};

    // Initialize with these parameters
    det->initialize(2, 2, std::move(x), std::move(y), true, 0, true, 2, 1);

    do_test_on(det);

    // --- Now make a parametrized version ----
    ParameterMap_sptr pmap(new ParameterMap());
    StructuredDetector *parDet = new StructuredDetector(det, pmap.get());

    do_test_on(parDet);

    delete det;
    delete parDet;
  }

  void testBeamDirectionIsZ() {
    StructuredDetector *det = new StructuredDetector("Detector");

    std::vector<double> x{0, 1, 2, 0, 1, 2, 0, 1, 2};
    std::vector<double> y{0, 0, 0, 1, 1, 1, 2, 2, 2};

    TSM_ASSERT_THROWS("StructuredDetectors created with beams not aligned "
                      "along the z-axis should fail.",
                      det->initialize(2, 2, std::move(x), std::move(y), false, 0, true, 2, 1),
                      const std::invalid_argument &);

    delete det;
  }

  void testIncorrectVertexArraySize() {
    auto cuboidShape = ComponentCreationHelper::createCuboid(0.5);

    StructuredDetector *det = new StructuredDetector("MyStructuredDetector");
    det->setPos(1000., 2000., 3000.);

    std::vector<double> x{0, 1, 2, 0, 1, 2};
    std::vector<double> y{0, 0, 0, 1, 1, 1};

    auto x2 = x;
    auto y2 = y;

    // Initialize with these parameters
    TS_ASSERT_THROWS(det->initialize(2, 2, std::move(x), std::move(y), true, 0, true, 2, 1),
                     const std::invalid_argument &);

    x2.resize(3);
    auto x3 = x2;
    auto y3 = y2;

    TS_ASSERT_THROWS(det->initialize(2, 2, std::move(x2), std::move(y2), true, 0, true, 2, 1),
                     const std::invalid_argument &);

    x3.resize(0);
    y3.resize(0);

    TS_ASSERT_THROWS(det->initialize(2, 2, std::move(x3), std::move(y3), true, 0, true, 2, 1),
                     const std::invalid_argument &);

    delete det;
  }

  /** Test on a structured detector that will be
   * repeated on an un-moved parametrized version.
   */
  void do_test_on(StructuredDetector *det) {
    TS_ASSERT_EQUALS(det->xPixels(), 2);
    TS_ASSERT_EQUALS(det->yPixels(), 2);

    auto size = (det->xPixels() + 1) * (det->yPixels() + 1);

    TS_ASSERT_EQUALS(det->getXValues().size(), size);
    TS_ASSERT_EQUALS(det->getYValues().size(), size);

    // Go out of bounds
    TS_ASSERT_THROWS(det->getAtXY(-1, 0), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXY(0, -1), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXY(5, 0), const std::runtime_error &);
    TS_ASSERT_THROWS(det->getAtXY(0, 6), const std::runtime_error &);

    // Check some ids
    TS_ASSERT_EQUALS(det->getAtXY(0, 0)->getID(), 0);
    TS_ASSERT_EQUALS(det->getAtXY(0, 1)->getID(), 1);
    TS_ASSERT_EQUALS(det->getAtXY(1, 1)->getID(), 3);

    std::pair<size_t, size_t> xy;
    size_t x;
    size_t y;

    TS_ASSERT_THROWS_NOTHING(xy = det->getXYForDetectorID(0));
    x = xy.first;
    y = xy.second;
    TS_ASSERT_EQUALS(x, 0);
    TS_ASSERT_EQUALS(y, 0);

    TS_ASSERT_THROWS_NOTHING(xy = det->getXYForDetectorID(1));
    x = xy.first;
    y = xy.second;
    TS_ASSERT_EQUALS(x, 0);
    TS_ASSERT_EQUALS(y, 1);

    TS_ASSERT_THROWS_NOTHING(xy = det->getXYForDetectorID(2));
    x = xy.first;
    y = xy.second;
    TS_ASSERT_EQUALS(x, 1);
    TS_ASSERT_EQUALS(y, 0);

    // Name
    TS_ASSERT_EQUALS(det->getAtXY(0, 1)->getName(), "MyStructuredDetector(0,1)");
    TS_ASSERT_EQUALS(det->getChild(1)->getName(), "MyStructuredDetector(x=1)");
  }

private:
  /// A detector of a single unit-square pixel with corners at (0, 0) and (1, 1).
  static std::unique_ptr<StructuredDetector> makeOnePixelDetector() {
    auto det = std::make_unique<StructuredDetector>("structured");
    std::vector<double> x{0, 1, 0, 1};
    std::vector<double> y{0, 0, 1, 1};
    det->initialize(1, 1, std::move(x), std::move(y), true, 0, true, 1, 1);
    return det;
  }
};

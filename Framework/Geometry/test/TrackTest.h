// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

class TrackTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    Track A(V3D(0, 0, 0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.startPoint(), V3D(0.0, 0.0, 0));
    TS_ASSERT_EQUALS(A.direction(), V3D(1.0, 0.0, 0.0));
  }

  void testTrackParamConstructor() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(A.direction(), V3D(1.0, 0.0, 0.0));
    Track B(A);
    TS_ASSERT_EQUALS(B.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(B.direction(), V3D(1.0, 0.0, 0.0));
  }

  void testIterator() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    Track::LType::const_iterator iterBegin = A.cbegin();
    Track::LType::const_iterator iterEnd = A.cend();
    TS_ASSERT_EQUALS(iterBegin, iterEnd);
  }

  void testAddLink() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    CSGObject shape;
    A.addLink(V3D(2, 2, 2), V3D(3, 3, 3), 2.0, shape, nullptr);
    const auto &linkFront = A.front();
    const auto &linkBack = A.back();
    TS_ASSERT_EQUALS(&linkFront, &linkBack);
    Track::LType::const_iterator iterBegin = A.cbegin();
    Track::LType::const_iterator iterEnd = A.cend();
    iterBegin++;
    TS_ASSERT_EQUALS(iterBegin, iterEnd);
  }

  void testreset() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(A.direction(), V3D(1.0, 0.0, 0.0));
    A.reset(V3D(2, 2, 2), V3D(0.0, 1.0, 0.0));
    TS_ASSERT_EQUALS(A.startPoint(), V3D(2.0, 2.0, 2));
    TS_ASSERT_EQUALS(A.direction(), V3D(0.0, 1.0, 0.0));
  }

  void testAssignment() { // Also have to test the Links and Points
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(A.direction(), V3D(1.0, 0.0, 0.0));
    Track B(V3D(2, 2, 2), V3D(0.0, 1.0, 0.0));
    TS_ASSERT_EQUALS(B.startPoint(), V3D(2.0, 2.0, 2));
    TS_ASSERT_EQUALS(B.direction(), V3D(0.0, 1.0, 0.0));
    B = A;
    TS_ASSERT_EQUALS(B.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(B.direction(), V3D(1.0, 0.0, 0.0));
  }

  void testBuildLink() {
    Track A(V3D(-5, -5, 0), V3D(1.0, 0.0, 0.0));
    CSGObject shape;

    TS_ASSERT_EQUALS(A.startPoint(), V3D(-5.0, -5.0, 0.0));
    TS_ASSERT_EQUALS(A.direction(), V3D(1.0, 0.0, 0.0));
    A.addPoint(TrackDirection::ENTERING, V3D(-5.0, -2.0, 0.0),
               shape); // Entry at -5,-2,0
    A.addPoint(TrackDirection::LEAVING, V3D(-5.0, 2.0, 0.0),
               shape); // Exit point at -5,2,0
    A.buildLink();
    // Check track length
    int index = 0;
    for (Track::LType::const_iterator it = A.cbegin(); it != A.cend(); ++it) {
      TS_ASSERT_DELTA(it->distFromStart, 7, 0.0001);
      TS_ASSERT_DELTA(it->distInsideObject, 4, 0.0001);
      TS_ASSERT_EQUALS(it->componentID, (Component *)nullptr);
      TS_ASSERT_EQUALS(it->entryPoint, V3D(-5.0, -2.0, 0.0));
      TS_ASSERT_EQUALS(it->exitPoint, V3D(-5.0, 2.0, 0.0));
      index++;
    }
    TS_ASSERT_EQUALS(index, 1);
  }

  void testRemoveCojoins() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    CSGObject shape;
    A.addLink(V3D(2, 2, 2), V3D(3, 3, 3), 2.0, shape);
    A.addLink(V3D(2.0001, 2.0001, 2.0001), V3D(3, 3, 3), 2.001, shape);
    // Check track length
    int index = 0;
    for (Track::LType::const_iterator it = A.cbegin(); it != A.cend(); ++it) {
      index++;
    }
    TS_ASSERT_EQUALS(index, 2);
    A.removeCojoins();
    index = 0;
    {
      for (Track::LType::const_iterator it = A.cbegin(); it != A.cend(); ++it) {
        index++;
      }
    }
    TS_ASSERT_EQUALS(index, 1);
  }

  void testNonComplete() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    CSGObject shape;
    A.addLink(V3D(2, 2, 2), V3D(3, 3, 3), 2.0, shape);
    A.addLink(V3D(2.0001, 2.0001, 2.0001), V3D(3, 3, 3), 2.001, shape);
    TS_ASSERT(A.nonComplete() > 0);
    Track B(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(B.startPoint(), V3D(1.0, 1.0, 1));
    TS_ASSERT_EQUALS(B.direction(), V3D(1.0, 0.0, 0.0));
    B.addLink(V3D(1, 1, 1), V3D(1, 3, 1), 0.0, shape);
    B.addLink(V3D(1, 3, 1), V3D(1, 5, 1), 2.0, shape);
    TS_ASSERT_EQUALS(B.nonComplete(), 0);
  }

  void testAddPoint() {
    Track A(V3D(1, 1, 1), V3D(1.0, 0.0, 0.0));
    CSGObject shape;
    // check point addition including duplicate removal
    A.addPoint(TrackDirection::ENTERING, V3D(-5.0, -2.0, 0.0),
               shape); // Entry at -5,-2,0
    A.addPoint(TrackDirection::LEAVING, V3D(-5.0, 2.0, 0.0),
               shape); // Exit point at -5,2,0
    A.addPoint(TrackDirection::LEAVING, V3D(-5.0, 2.0, 0.0),
               shape); // Duplicate exit point at -5,2,0
    TS_ASSERT_EQUALS(A.surfPointsCount(), 2);

    // check point addition works on reused track
    A.clearIntersectionResults();
    A.addPoint(TrackDirection::LEAVING, V3D(-5.0, 2.0, 0.0),
               shape); // Exit point at -5,2,0
    A.addPoint(TrackDirection::ENTERING, V3D(-5.0, -2.0, 0.0),
               shape); // Entry at -5,-2,0
    TS_ASSERT_EQUALS(A.surfPointsCount(), 2);
  }

  void test_calculateAttenuation() {
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    auto shape = ComponentCreationHelper::createSphere(0.1);
    shape->setMaterial(Kernel::Material("Vanadium", Mantid::PhysicalConstants::getNeutronAtom(23), 0.02));
    // use tracks generated by
    // MCInteractionVolume::test_Solid_Sample_Gives_Expected_Tracks test
    Track beforeScatter({-0.05, -0.05, -0.05}, {-0.999343185, 0.025624184, 0.025624184});
    beforeScatter.addLink({-0.05, -0.05, -0.05}, {-0.071481137, -0.049449202, -0.049449202}, 0.021495255, *shape);
    Track afterScatter({-0.05, -0.05, -0.05}, {0.417472754, 0.417472755, 0.807113993});
    afterScatter.addLink({-0.05, -0.05, -0.05}, {0.024407241, 0.024407241, 0.093853999}, 0.021495255, *shape);
    const double factor =
        beforeScatter.calculateAttenuation(lambdaBefore) * afterScatter.calculateAttenuation(lambdaAfter);
    TS_ASSERT_DELTA(0.0028357258, factor, 1e-8);
  }
};

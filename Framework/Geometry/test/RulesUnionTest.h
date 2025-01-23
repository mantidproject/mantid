// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#include <cfloat>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <vector>

#include "boost/make_shared.hpp"

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

class RulesUnionTest : public CxxTest::TestSuite {
public:
  void testDefaultConstructor() {
    Union A;
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)nullptr);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)nullptr);
  }

  void testTwoRuleConstructor() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
  }

  void testThreeRuleConstructor() {
    Union Parent;
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(&Parent, std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    TS_ASSERT_EQUALS(A.getParent(), &Parent);
  }

  void testUnionConstructor() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
  }

  void testClone() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    auto B = A.clone();
    TS_ASSERT_EQUALS(B->display(), "10 : 11");
    TS_ASSERT_EQUALS(B->leaf(0)->display(), ptrS1->display());
    TS_ASSERT_EQUALS(B->leaf(1)->display(), ptrS2->display());
  }

  void testAssignment() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
  }

  void testSetLeaves() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A;
    A.setLeaves(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
  }

  void testSetLeaf() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A;
    A.setLeaf(std::move(S2), 1);
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)nullptr);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    A.setLeaf(std::move(S1), 0);
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
  }

  void testFindLeaf() {
    SurfPoint S3;
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A;
    A.setLeaves(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    TS_ASSERT_EQUALS(A.findLeaf(ptrS1), 0);
    TS_ASSERT_EQUALS(A.findLeaf(ptrS2), 1);
    TS_ASSERT_EQUALS(A.findLeaf(&S3), -1);
  }

  void testFindKey() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A;
    A.setLeaves(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    TS_ASSERT_EQUALS(A.findKey(10), ptrS1);
    TS_ASSERT_EQUALS(A.findKey(11), ptrS2);
    TS_ASSERT_EQUALS(A.findKey(15), (Rule *)nullptr);
  }

  void testIsComplementary() { // Problem: it only detects whether first leaf or
                               // second leaf but not both
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : 11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    TS_ASSERT_EQUALS(A.isComplementary(), 0);
    auto B = std::make_unique<CompObj>();
    auto C = std::make_unique<CompObj>();
    A.setLeaf(std::move(B), 1);
    TS_ASSERT_EQUALS(A.isComplementary(), -1);
    A.setLeaf(std::move(C), 0);
    TS_ASSERT_EQUALS(A.isComplementary(), 1);
  }

  void testIsValid() {

    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(-11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : -11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    TS_ASSERT_EQUALS(A.isValid(V3D(0.0, 0.0, 0.0)), true); // on surface
    TS_ASSERT_EQUALS(A.isValid(V3D(5.0, 0.0, 0.0)), true); // inside surface
    TS_ASSERT_EQUALS(A.isValid(V3D(-0.1, 0.0, 0.0)),
                     false); // just outside surface
    TS_ASSERT_EQUALS(A.isValid(V3D(10.1, 1.0, 1.0)),
                     true); // on other side of the plane
  }

  void testIsValidMap() {
    auto P1 = std::make_shared<Plane>();
    auto Sp1 = std::make_shared<Sphere>();
    P1->setSurface("px 5");             // yz plane with x=5
    Sp1->setSurface("s 5.0 0.0 0.0 5"); // a sphere with center (5,0,0) and
                                        // radius 5. this will touch origin
    auto S1 = std::make_unique<SurfPoint>();
    auto S2 = std::make_unique<SurfPoint>();
    S1->setKey(P1);
    S1->setKeyN(10);
    S2->setKey(Sp1);
    S2->setKeyN(-11);
    auto ptrS1 = S1.get();
    auto ptrS2 = S2.get();
    Union A(std::move(S1), std::move(S2));
    TS_ASSERT_EQUALS(A.display(), "10 : -11");
    TS_ASSERT_EQUALS(A.leaf(0), ptrS1);
    TS_ASSERT_EQUALS(A.leaf(1), ptrS2);
    std::map<int, int> input;
    input[5] = 1;
    input[10] = 1;
    input[11] = 1;
    input[15] = 0;
    input[20] = -1;
    TS_ASSERT_EQUALS(A.isValid(input), true);
    input[10] = 0;
    input[11] = 0;
    TS_ASSERT_EQUALS(A.isValid(input), true);
    input[11] = 1;
    TS_ASSERT_EQUALS(A.isValid(input), false);
  }
};

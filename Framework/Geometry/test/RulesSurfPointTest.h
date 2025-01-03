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

class RulesSurfPointTest : public CxxTest::TestSuite {
public:
  void testDefaultConstructor() {
    SurfPoint A;
    TS_ASSERT_EQUALS(A.display(), "0");
  }

  void testSetKey() {
    SurfPoint A;
    TS_ASSERT_EQUALS(A.display(), "0");
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
  }

  void testSetKeyN() {
    SurfPoint A;
    TS_ASSERT_EQUALS(A.getKeyN(), 0);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
  }

  void testFullCreatedObject() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
  }

  void testSimplyfy() { // A simple leafnode of SurfPoint cannot be simplefield
                        // so it should always return 0
    SurfPoint A;
    TS_ASSERT_EQUALS(A.simplify(), 0);
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.simplify(), 0);
  }

  void testLeaf() { // SurfPoint will always be end of tree always returns 0
    SurfPoint A;
    TS_ASSERT_EQUALS(A.simplify(), 0);
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)nullptr);
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.simplify(), 0);
    TS_ASSERT_EQUALS(A.leaf(10), (Rule *)nullptr);
  }

  void testSetLeaves() { // This should not enabled as surfpoint cannot have leaves
    // TODO:disable setleaves function
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    SurfPoint B;
    auto S1 = std::make_shared<Sphere>();
    B.setKey(S1);
    B.setKeyN(11);
    TS_ASSERT_EQUALS(B.getKey(), S1.get());
    TS_ASSERT_EQUALS(B.getKeyN(), 11);
    TS_ASSERT_EQUALS(B.display(), "11");
    A.setLeaves(B.clone(), std::unique_ptr<Rule>());
    TS_ASSERT(dynamic_cast<Sphere *>(A.getKey()));
    TS_ASSERT_EQUALS(A.getKeyN(), 11);
    TS_ASSERT_EQUALS(A.display(), "11");
  }

  void testSetLeaf() { // TODO: disable setleaf function as surfpoint cannot
                       // have leaves
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    SurfPoint B;
    auto S1 = std::make_shared<Sphere>();
    B.setKey(S1);
    B.setKeyN(11);
    TS_ASSERT_EQUALS(B.getKey(), S1.get());
    TS_ASSERT_EQUALS(B.getKeyN(), 11);
    TS_ASSERT_EQUALS(B.display(), "11");
    A.setLeaf(B.clone(), 0);
    TS_ASSERT(dynamic_cast<Sphere *>(A.getKey()));
    TS_ASSERT_EQUALS(A.getKeyN(), 11);
    TS_ASSERT_EQUALS(A.display(), "11");
  }

  void testfindLeaf() { // TODO: disable to find leaf as this is final
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    SurfPoint B;
    auto S1 = std::make_shared<Sphere>();
    B.setKey(S1);
    B.setKeyN(11);
    TS_ASSERT_EQUALS(B.getKey(), S1.get());
    TS_ASSERT_EQUALS(B.getKeyN(), 11);
    TS_ASSERT_EQUALS(B.display(), "11");
    TS_ASSERT_EQUALS(A.findLeaf(&B), -1);
    B.setKey(std::make_shared<Plane>());
    B.setKeyN(10);
    TS_ASSERT_EQUALS(A.findLeaf(&B), -1);
    TS_ASSERT_EQUALS(A.findLeaf(&A), 0);
  }

  void testFindKey() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.findKey(10), &A);
    TS_ASSERT_EQUALS(A.findKey(11), (Rule *)nullptr);
  }

  void testGetSign() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    A.setKeyN(-10);
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.getSign(), -1);
  }

  void testSelfConstructor() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    SurfPoint B(A);
    TS_ASSERT(dynamic_cast<Plane *>(B.getKey()));
    TS_ASSERT_EQUALS(B.getKeyN(), 10);
    TS_ASSERT_EQUALS(B.display(), "10");
    TS_ASSERT_EQUALS(B.getSign(), 1);
  }

  void testClone() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    auto B = A.clone();
    TS_ASSERT(dynamic_cast<Plane *>(B->getKey()));
    TS_ASSERT_EQUALS(B->getKeyN(), 10);
    TS_ASSERT_EQUALS(B->display(), "10");
    TS_ASSERT_EQUALS(B->getSign(), 1);
  }

  void testAssignment() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    SurfPoint B;
    B = A;
    TS_ASSERT(dynamic_cast<Plane *>(B.getKey()));
    TS_ASSERT_EQUALS(B.getKeyN(), 10);
    TS_ASSERT_EQUALS(B.display(), "10");
    TS_ASSERT_EQUALS(B.getSign(), 1);
  }

  void testIsValid() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    P1->setSurface("px 5");
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    TS_ASSERT_EQUALS(A.isValid(V3D(4.9, 0.0, 0.0)), false);
    TS_ASSERT_EQUALS(A.isValid(V3D(5, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(A.isValid(V3D(5.1, 0.0, 0.0)), true);
  }

  void testIsValidMap() {
    SurfPoint A;
    auto P1 = std::make_shared<Plane>();
    P1->setSurface("px 5");
    A.setKey(P1);
    A.setKeyN(10);
    TS_ASSERT_EQUALS(A.getKey(), P1.get());
    TS_ASSERT_EQUALS(A.getKeyN(), 10);
    TS_ASSERT_EQUALS(A.display(), "10");
    TS_ASSERT_EQUALS(A.getSign(), 1);
    std::map<int, int> input;
    input[5] = 1;
    input[10] = 1;
    input[15] = 0;
    input[20] = -1;
    TS_ASSERT_EQUALS(A.isValid(input), true);
    A.setKeyN(15);
    TS_ASSERT_EQUALS(A.isValid(input), false);
    A.setKeyN(20);
    TS_ASSERT_EQUALS(A.isValid(input), true);
  }
};
//---------------------------------End of
// SurfPoint-------------------------------------

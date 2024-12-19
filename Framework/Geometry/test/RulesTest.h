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

class RulesTest : public CxxTest::TestSuite {
public:
  void testMakeCNFCopy() {}

  void testMakeFullDNF() {}

  void testMakeCNF() {
    auto tree = createAMixedTree();
    TS_ASSERT_EQUALS(tree->display(), "(10 11) : (12 10)");
    // TS_ASSERT_EQUALS(Rule::makeCNFcopy(tree),1);
    // TS_ASSERT_EQUALS(tree->display(),"(10 11) : (12 10)");
  }

  void testRemoveComplementary() {
    auto tree = createAUnionTree();
    TS_ASSERT_EQUALS(tree->display(), "10 : 10 : 12 : 11");
    TS_ASSERT_EQUALS(tree->removeComplementary(tree), 1);
    //		TS_ASSERT_EQUALS(tree->display(),"10 : 12 : 11"); //Problem: The
    // comments don't match to the functionaility it is supposed to do
  }

  void testRemoveItem() { // Problem: in removing the item of the tree that has
    // more than one instances,possibly double deletion
    auto tree = createAUnionTree();
    TS_ASSERT_EQUALS(tree->removeItem(tree, 11), 1);
    //		TS_ASSERT_EQUALS(tree->removeItem(tree,10),2);
    TS_ASSERT_EQUALS(tree->removeItem(tree, 11), 0);
    TS_ASSERT_EQUALS(tree->removeItem(tree, 12), 1);
  }

  void testCommonType() {
    auto uTree = createAUnionTree();
    TS_ASSERT_EQUALS(uTree->commonType(), -1);
    auto iTree = createAIntersectionTree();
    TS_ASSERT_EQUALS(iTree->commonType(), 1);
    auto mTree = createAMixedTree();
    TS_ASSERT_EQUALS(mTree->commonType(), 0);
  }

  void testSubstituteSurf() {
    auto uTree = createAUnionTree();
    TS_ASSERT_EQUALS(uTree->substituteSurf(11, 13, std::make_shared<Cone>()), 1);
    TS_ASSERT_EQUALS(uTree->display(), "10 : 10 : 12 : 13");
    TS_ASSERT_EQUALS(uTree->substituteSurf(10, 14, std::make_shared<Sphere>()),
                     2); // its suppose to return 2
    TS_ASSERT_EQUALS(uTree->display(), "14 : 14 : 12 : 13");
  }

  void testEliminate() {}

private:
  std::unique_ptr<Rule> createAUnionTree() { // A:B:C:A
                                             // A Node
    // SurfPoint *A, *B, *C;
    auto A = std::make_unique<SurfPoint>();
    A->setKey(std::make_shared<Plane>());
    A->setKeyN(10);
    auto B = std::make_unique<SurfPoint>();
    B->setKey(std::make_shared<Sphere>());
    B->setKeyN(11);
    auto C = std::make_unique<SurfPoint>();
    C->setKey(std::make_shared<Cylinder>());
    C->setKeyN(12);

    auto Left = std::make_unique<Union>(A->clone(), A->clone());
    auto Right = std::make_unique<Union>(std::move(C), std::move(B));
    return std::make_unique<Union>(std::move(Left), std::move(Right));
  }

  std::unique_ptr<Rule> createAIntersectionTree() { // A B C A
    // A Node
    auto A = std::make_unique<SurfPoint>();
    A->setKey(std::make_shared<Plane>());
    A->setKeyN(10);
    auto B = std::make_unique<SurfPoint>();
    B->setKey(std::make_shared<Sphere>());
    B->setKeyN(11);
    auto C = std::make_unique<SurfPoint>();
    C->setKey(std::make_shared<Cylinder>());
    C->setKeyN(12);

    auto Left = std::make_unique<Intersection>();

    Left->setLeaves(A->clone(), std::move(B));

    auto Right = std::make_unique<Intersection>();
    Right->setLeaves(std::move(C), std::move(A));

    return std::make_unique<Intersection>(std::move(Left), std::move(Right));
  }

  std::unique_ptr<Rule> createAMixedTree() { // A B : C A
    auto A = std::make_unique<SurfPoint>();
    A->setKey(std::make_shared<Plane>());
    A->setKeyN(10);
    auto B = std::make_unique<SurfPoint>();
    B->setKey(std::make_shared<Sphere>());
    B->setKeyN(11);
    auto C = std::make_unique<SurfPoint>();
    C->setKey(std::make_shared<Cylinder>());
    C->setKeyN(12);

    auto Root = std::make_unique<Union>();

    auto Left = std::make_unique<Intersection>();
    Left->setLeaves(A->clone(), std::move(B));

    auto Right = std::make_unique<Intersection>();
    Right->setLeaves(std::move(C), std::move(A));

    Root->setLeaves(std::move(Left), std::move(Right));
    return Root;
  }
};

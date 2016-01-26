#ifndef MANTID_RULESBOOLVALUE_TEST__
#define MANTID_RULESBOOLVALUE_TEST__
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include <cfloat>
#include "MantidKernel/V3D.h"
#include "MantidKernel/make_unique.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

//---------------------------------End of
// CompGrp----------------------------------------
class RulesBoolValueTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    A.setStatus(1);
    TS_ASSERT_EQUALS(A.display(), " True ");
  }

  void testBoolValueConstructor() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
  }

  void testClone() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    auto B = A.clone();
    TS_ASSERT_EQUALS(B->leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(B->leaf(1), (Rule *)0);
    TS_ASSERT_EQUALS(B->display(), " False ");
  }

  void testAssignment() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
  }

  void testLeafOperations() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    auto B = Mantid::Kernel::make_unique<BoolValue>();
    TS_ASSERT_EQUALS(B->display(), " Unknown ");
    B->setStatus(1);
    A.setLeaves(std::move(B), std::unique_ptr<Rule>());
    TS_ASSERT_EQUALS(A.display(), " True ");
    auto C = Mantid::Kernel::make_unique<BoolValue>();
    TS_ASSERT_EQUALS(C->display(), " Unknown ");
    C->setStatus(0);
    A.setLeaf(std::move(C), 1);
    TS_ASSERT_EQUALS(A.display(), " False ");
  }

  void testFindOperations() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    auto B = Mantid::Kernel::make_unique<BoolValue>();
    TS_ASSERT_EQUALS(B->display(), " Unknown ");
    B->setStatus(1);
    Rule *ptrB = B.get();
    A.setLeaves(std::move(B), std::unique_ptr<Rule>());
    TS_ASSERT_EQUALS(A.display(), " True ");
    TS_ASSERT_EQUALS(A.findLeaf(&A), 0);
    TS_ASSERT_EQUALS(A.findLeaf(ptrB), -1);
    TS_ASSERT_EQUALS(A.findKey(0), (Rule *)0);
  }

  void testIsValid() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    TS_ASSERT_EQUALS(A.isValid(V3D(0, 0, 0)), false);
    A.setStatus(-1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0, 0, 0)), false);
    A.setStatus(1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0, 0, 0)), true);

    std::map<int, int> input;
    input[0] = 0;
    input[5] = 1;
    input[10] = 1;
    input[15] = 0;
    input[20] = -1;
    TS_ASSERT_EQUALS(A.isValid(input), true);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.isValid(input), false);
  }
  void testSimplyfy() {
    BoolValue A;
    TS_ASSERT_EQUALS(A.display(), " Unknown ");
    TS_ASSERT_EQUALS(A.leaf(0), (Rule *)0);
    TS_ASSERT_EQUALS(A.leaf(1), (Rule *)0);
    A.setStatus(0);
    TS_ASSERT_EQUALS(A.display(), " False ");
    TS_ASSERT_EQUALS(A.simplify(),
                     0); // Always return 0 bcos a end node cannot be simplified
  }
};

#endif

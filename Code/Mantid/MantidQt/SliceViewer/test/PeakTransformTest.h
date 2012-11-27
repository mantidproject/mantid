#ifndef SLICE_VIEWER_PEAKTRANSFORM_TEST_H_
#define SLICE_VIEWER_PEAKTRANSFORM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakTransform.h"

using namespace MantidQt::SliceViewer;
using namespace Mantid;
using Mantid::Kernel::V3D;
//using namespace testing;

class PeakTransformTest : public CxxTest::TestSuite
{
public:

  void test_throws_with_unknown_xLabel()
  {
    TS_ASSERT_THROWS(PeakTransform("?", "K (Lattice)"), std::runtime_error);
  }

  void test_throws_with_unknown_yLabel()
  {
    TS_ASSERT_THROWS(PeakTransform("H (Lattice)", "?"), std::runtime_error);
  }

void test_transformHKL()
{
  PeakTransform transform("H (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(original.X(), original.X());
  TS_ASSERT_EQUALS(original.Y(), original.Y());
  TS_ASSERT_EQUALS(original.Z(), original.Z());
}

void test_transformHLK()
{
  PeakTransform transform("H (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.X()); // X -> H
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K
}

void test_transformLKH()
{

  PeakTransform transform("L (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.Y()); // Y -> K
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H
}

void test_transformLHK()
{
  PeakTransform transform("L (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K
}

void test_transformKLH()
{
  PeakTransform transform("K (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H
}

void test_transformKHL()
{
  PeakTransform transform("K (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Z()); // Z -> L
}

void test_copy_construction()
{
  PeakTransform A("H", "L");
  PeakTransform B(A);

  // Test indirectly via what the transformations produce.
  V3D productA = A.transform(V3D(0, 1, 2));
  V3D productB = B.transform(V3D(0, 1, 2));
  TS_ASSERT_EQUALS(productA, productB);
}

void test_assigment()
{
}

private:
};
#endif

//end SLICE_VIEWER_PEAKTRANSFORM_TEST_H_
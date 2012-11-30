#ifndef SLICE_VIEWER_PEAKTRANSFORM_TEST_H_
#define SLICE_VIEWER_PEAKTRANSFORM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakTransformHKL.h"

using namespace MantidQt::SliceViewer;
using namespace Mantid;
using Mantid::Kernel::V3D;
//using namespace testing;

class PeakTransformHKLTest : public CxxTest::TestSuite
{
public:

  void test_throws_with_unknown_xLabel()
  {
    TS_ASSERT_THROWS(PeakTransformHKL("?", "K (Lattice)"), PeakTransformException);
  }

  void test_throws_with_unknown_yLabel()
  {
    TS_ASSERT_THROWS(PeakTransformHKL("H (Lattice)", "?"), PeakTransformException);
  }

void test_transformHKL()
{
  PeakTransformHKL transform("H (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.X());
  TS_ASSERT_EQUALS(transformed.Y(), original.Y());
  TS_ASSERT_EQUALS(transformed.Z(), original.Z());

  boost::regex_match("L (Lattice)", transform.getFreePeakAxisRegex());
}

void test_transformHLK()
{
  PeakTransformHKL transform("H (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.X()); // X -> H
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K

  boost::regex_match("K (Lattice)", transform.getFreePeakAxisRegex());
}

void test_transformLKH()
{
  PeakTransformHKL transform("L (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.Y()); // Y -> K
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

  boost::regex_match("H (Lattice)", transform.getFreePeakAxisRegex());
}

void test_transformLHK()
{
  PeakTransformHKL transform("L (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K

  boost::regex_match("K (Lattice)", transform.getFreePeakAxisRegex());
}

void test_transformKLH()
{
  PeakTransformHKL transform("K (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

  boost::regex_match("H (Lattice)", transform.getFreePeakAxisRegex());
}

void test_transformKHL()
{
  PeakTransformHKL transform("K (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Z()); // Z -> L

  boost::regex_match("L (Lattice)", transform.getFreePeakAxisRegex());
}

void test_copy_construction()
{
  PeakTransformHKL A("H", "L");
  PeakTransformHKL B(A);

  // Test indirectly via what the transformations produce.
  V3D productA = A.transform(V3D(0, 1, 2));
  V3D productB = B.transform(V3D(0, 1, 2));
  TS_ASSERT_EQUALS(productA, productB);  
  // Test indirectly via the free regex.
  boost::regex regexA = A.getFreePeakAxisRegex();
  boost::regex regexB = B.getFreePeakAxisRegex();
  TS_ASSERT_EQUALS(regexA, regexB);
 
}

void test_assigment()
{
  PeakTransformHKL A("H", "L");
  PeakTransformHKL B("K", "H");
  A = B;

  // Test indirectly via what the transformations produce.
  V3D productA = A.transform(V3D(0, 1, 2));
  V3D productB = B.transform(V3D(0, 1, 2));
  TS_ASSERT_EQUALS(productA, productB);  
  // Test indirectly via the free regex.
  boost::regex regexA = A.getFreePeakAxisRegex();
  boost::regex regexB = B.getFreePeakAxisRegex();
  TS_ASSERT_EQUALS(regexA, regexB);
}

private:
};
#endif

//end SLICE_VIEWER_PEAKTRANSFORM_TEST_H_
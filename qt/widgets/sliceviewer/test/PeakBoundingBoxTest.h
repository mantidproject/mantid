#ifndef SLICE_VIEWER_PEAKBOUNDINGBOX_TEST_H_
#define SLICE_VIEWER_PEAKBOUNDINGBOX_TEST_H_

#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::SliceViewer;

class PeakBoundingBoxTest : public CxxTest::TestSuite {

public:
  void test_construction() {
    const Right expectedRight(1);
    const Left expectedLeft(0);
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);

    PeakBoundingBox box(expectedLeft, expectedRight, expectedTop,
                        expectedBottom, expectedSlicePoint);

    TS_ASSERT_EQUALS(expectedLeft(), box.left());
    TS_ASSERT_EQUALS(expectedRight(), box.right());
    TS_ASSERT_EQUALS(expectedTop(), box.top());
    TS_ASSERT_EQUALS(expectedBottom(), box.bottom());
    TS_ASSERT_EQUALS(expectedSlicePoint(), box.slicePoint());
    TS_ASSERT_EQUALS(expectedSlicePoint(), box.front());
    TS_ASSERT_EQUALS(expectedSlicePoint(), box.back());
  }

  void test_full_construction() {
    const Right expectedRight(1);
    const Left expectedLeft(0);
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);
    const Front expectedFront(0);
    const Back expectedBack(1);

    PeakBoundingBox box(expectedLeft, expectedRight, expectedTop,
                        expectedBottom, expectedSlicePoint, expectedFront,
                        expectedBack);

    TS_ASSERT_EQUALS(expectedLeft(), box.left());
    TS_ASSERT_EQUALS(expectedRight(), box.right());
    TS_ASSERT_EQUALS(expectedTop(), box.top());
    TS_ASSERT_EQUALS(expectedBottom(), box.bottom());
    TS_ASSERT_EQUALS(expectedSlicePoint(), box.slicePoint());
    TS_ASSERT_EQUALS(expectedFront(), box.front());
    TS_ASSERT_EQUALS(expectedBack(), box.back());
  }

  void test_default_construction() {
    PeakBoundingBox box;

    TS_ASSERT_EQUALS(0, box.left());
    TS_ASSERT_EQUALS(0, box.right());
    TS_ASSERT_EQUALS(0, box.top());
    TS_ASSERT_EQUALS(0, box.bottom());
    TS_ASSERT_EQUALS(0, box.slicePoint());
    TS_ASSERT_EQUALS(0, box.front());
    TS_ASSERT_EQUALS(0, box.back());
  }

  void test_top_greater_than_bottom() {

    const Right expectedRight(1);
    const Left expectedLeft(0);
    const Top expectedTop(1);
    const Bottom expectedBottom(2); // oops top < bottom
    const SlicePoint expectedSlicePoint(1);

    TSM_ASSERT_THROWS("Top < Bottom",
                      PeakBoundingBox box(expectedLeft, expectedRight,
                                          expectedTop, expectedBottom,
                                          expectedSlicePoint),
                      std::invalid_argument &);
  }

  void test_right_greater_than_left() {
    const Right expectedRight(1);
    const Left expectedLeft(2); // oops right < left
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);

    TSM_ASSERT_THROWS("Right < Left",
                      PeakBoundingBox box(expectedLeft, expectedRight,
                                          expectedTop, expectedBottom,
                                          expectedSlicePoint),
                      std::invalid_argument &);
  }

  void test_front_greater_than_back() {
    const Right expectedRight(1);
    const Left expectedLeft(2);
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);
    const Front expectedFront(1);
    const Back expectedBack(0); // oops front > back.

    TSM_ASSERT_THROWS("Right < Left",
                      PeakBoundingBox box(expectedLeft, expectedRight,
                                          expectedTop, expectedBottom,
                                          expectedSlicePoint, expectedFront,
                                          expectedBack),
                      std::invalid_argument &);
  }

  void test_copy() {
    const Right expectedRight(1);
    const Left expectedLeft(0);
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);
    const Front expectedFront(0);
    const Back expectedBack(1);

    PeakBoundingBox A(expectedLeft, expectedRight, expectedTop, expectedBottom,
                      expectedSlicePoint, expectedFront, expectedBack);
    PeakBoundingBox B(A);

    TS_ASSERT_EQUALS(A.left(), B.left());
    TS_ASSERT_EQUALS(A.right(), B.right());
    TS_ASSERT_EQUALS(A.top(), B.top());
    TS_ASSERT_EQUALS(A.bottom(), B.bottom());
    TS_ASSERT_EQUALS(A.slicePoint(), B.slicePoint());
    TS_ASSERT_EQUALS(A.front(), B.front());
    TS_ASSERT_EQUALS(A.back(), B.back());
  }

  void test_assign() {
    const Right expectedRight(1);
    const Left expectedLeft(0);
    const Top expectedTop(1);
    const Bottom expectedBottom(0);
    const SlicePoint expectedSlicePoint(1);
    const Front expectedFront(0);
    const Back expectedBack(1);

    PeakBoundingBox A(expectedLeft, expectedRight, expectedTop, expectedBottom,
                      expectedSlicePoint, expectedFront, expectedBack);
    PeakBoundingBox B;

    B = A;

    TS_ASSERT_EQUALS(A.left(), B.left());
    TS_ASSERT_EQUALS(A.right(), B.right());
    TS_ASSERT_EQUALS(A.top(), B.top());
    TS_ASSERT_EQUALS(A.bottom(), B.bottom());
    TS_ASSERT_EQUALS(A.slicePoint(), B.slicePoint());
    TS_ASSERT_EQUALS(A.front(), B.front());
    TS_ASSERT_EQUALS(A.back(), B.back());
  }

  void test_are_equal() {
    PeakBoundingBox a(Left(-1), Right(1), Top(1), Bottom(-1), SlicePoint(5),
                      Front(3), Back(6));
    PeakBoundingBox b(Left(-1), Right(1), Top(1), Bottom(-1), SlicePoint(5),
                      Front(3), Back(6));

    TS_ASSERT(a == b);
  }

  void test_are_not_equal() {
    PeakBoundingBox a;
    PeakBoundingBox b(Left(-1), Right(0), Top(0), Bottom(0), SlicePoint(0),
                      Front(0), Back(0));
    PeakBoundingBox c(Left(0), Right(1), Top(0), Bottom(0), SlicePoint(0),
                      Front(0), Back(0));
    PeakBoundingBox d(Left(0), Right(0), Top(1), Bottom(0), SlicePoint(0),
                      Front(0), Back(0));
    PeakBoundingBox e(Left(0), Right(0), Top(0), Bottom(-1), SlicePoint(0),
                      Front(0), Back(0));
    PeakBoundingBox f(Left(0), Right(0), Top(0), Bottom(0), SlicePoint(1),
                      Front(0), Back(1));
    PeakBoundingBox g(Left(0), Right(0), Top(0), Bottom(0), SlicePoint(1),
                      Front(1), Back(1));
    PeakBoundingBox h(Left(0), Right(0), Top(0), Bottom(0), SlicePoint(0),
                      Front(0), Back(1));

    TS_ASSERT(a != b);
    TS_ASSERT(a != c);
    TS_ASSERT(a != d);
    TS_ASSERT(a != e);
    TS_ASSERT(a != f);
    TS_ASSERT(a != g);
    TS_ASSERT(a != h);
  }

  void test_to_string() {
    PeakBoundingBox box(Left(-1.234), Right(1.234), Top(2.234), Bottom(-20.234),
                        SlicePoint(3.2), Front(3.124), Back(4.123));
    const std::string extents = box.toExtentsString();
    TS_ASSERT_EQUALS("-1.23,1.23,-20.23,2.23,3.12,4.12", extents);
  }
};

#endif

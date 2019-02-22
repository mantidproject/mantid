// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_V2DTEST_H_
#define MANTID_KERNEL_V2DTEST_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>
#include <limits>

using Mantid::Kernel::V2D;
using Mantid::Kernel::V3D;

class V2DTest : public CxxTest::TestSuite {
public:
  void test_That_A_Default_Object_Is_At_The_Origin() {
    V2D origin;
    TS_ASSERT_EQUALS(origin.X(), 0.0);
    TS_ASSERT_EQUALS(origin.Y(), 0.0);
  }

  void test_That_XY_Value_Construction_Yields_Correct_Values() {
    V2D topRight(1, 2);
    TS_ASSERT_EQUALS(topRight.X(), 1.0);
    TS_ASSERT_EQUALS(topRight.Y(), 2.0);

    V2D topLeft(-1, 2);
    TS_ASSERT_EQUALS(topLeft.X(), -1.0);
    TS_ASSERT_EQUALS(topLeft.Y(), 2.0);

    V2D botRight(1, -2);
    TS_ASSERT_EQUALS(botRight.X(), 1.0);
    TS_ASSERT_EQUALS(botRight.Y(), -2.0);

    V2D botLeft(-1, -2);
    TS_ASSERT_EQUALS(botLeft.X(), -1.0);
    TS_ASSERT_EQUALS(botLeft.Y(), -2.0);
  }

  void test_That_Construction_From_Another_V2D_Gives_Object_With_Same_Values() {
    V2D first(5, 10);
    V2D second(first);
    TS_ASSERT_EQUALS(second.X(), first.X());
    TS_ASSERT_EQUALS(second.Y(), first.Y());
  }

  void test_That_Index_Operator_Gives_Back_The_Correct_Value() {
    V2D point(5, 10);
    TS_ASSERT_EQUALS(point[0], 5.0);
    TS_ASSERT_EQUALS(point[1], 10.0);
  }

  void test_Sum_Gives_Correct_Vector() {
    V2D p1(3, 4);
    V2D p2(4, 5);
    TS_ASSERT_EQUALS(p1 + p2, V2D(7.0, 9.0));
    // Symmetry
    TS_ASSERT_EQUALS(p2 + p1, V2D(7.0, 9.0));
  }

  void test_Inplace_Sum_Updates_LHS() {
    V2D p1(3, 4);
    V2D p2(4, 5);
    p1 += p2;
    TS_ASSERT_EQUALS(p1, V2D(7.0, 9.0));
    // p2 unchanged
    TS_ASSERT_EQUALS(p2, V2D(4.0, 5.0));
  }

  void test_Subtract_Gives_Correct_Vector() {
    V2D p1(3, 9);
    V2D p2(4, 5);
    TS_ASSERT_EQUALS(p1 - p2, V2D(-1.0, 4.0));
    // Anti-symmetry
    TS_ASSERT_EQUALS(p2 - p1, V2D(1.0, -4.0));
  }

  void test_Inplace_Subtract_Updates_LHS() {
    V2D p1(3, 9);
    V2D p2(4, 5);
    p1 -= p2;
    TS_ASSERT_EQUALS(p1, V2D(-1.0, 4.0));
    // p2 unchanged
    TS_ASSERT_EQUALS(p2, V2D(4.0, 5.0));
  }

  void test_Multiply_By_Double_Gives_Correct_Vector() {
    V2D p1(3, 9);
    TS_ASSERT_EQUALS(p1 * 4.0, V2D(12.0, 36.0));
  }

  void test_Inplace_Multiply_By_Double_Updates_LHS() {
    V2D p1(3, 9);
    p1 *= 3.0;
    TS_ASSERT_EQUALS(p1, V2D(9.0, 27.0));
  }

  void test_Negate_Gives_Same_Length_But_Opposite_Direction() {
    const V2D p1(-3, 9);
    const V2D p2 = -p1;
    TS_ASSERT_EQUALS(p2, V2D(3, -9))
  }

  void test_Negate_Works_With_Special_Values() {
    const V2D p1(INFINITY, std::nan(""));
    const V2D p2 = -p1;
    TS_ASSERT_EQUALS(p2.X(), -INFINITY);
    TS_ASSERT(std::isnan(p2.Y()));
  }

  void test_Equality_Gives_True_When_Diff_Less_Than_Tolerance() {
    const double tolerance = std::numeric_limits<double>::epsilon();
    V2D first(5, 10);
    V2D second(5 + 0.5 * tolerance, 10 - 0.5 * tolerance);
    TS_ASSERT_EQUALS(first, second);
    TS_ASSERT(!(first != second));
  }

  void test_Equality_Gives_False_When_Diff_More_Than_Tolerance() {
    const double tolerance = std::numeric_limits<double>::epsilon();
    V2D first(5, 10);
    V2D second(5 + 0.5 * tolerance, 11);
    TS_ASSERT(!(first == second));

    second = V2D(6, 10 + 0.5 * tolerance);
    TS_ASSERT(!(first == second));
  }

  void test_Call_To_Normalize_Gives_Unit_Vector_After_Call() {
    V2D diag(1, 1);
    TS_ASSERT_DELTA(diag.normalize(), M_SQRT2, DBL_EPSILON);
    TS_ASSERT_DELTA(diag.X(), M_SQRT1_2, DBL_EPSILON);
    TS_ASSERT_DELTA(diag.Y(), M_SQRT1_2, DBL_EPSILON);
  }

  void test_Norm_Gives_Length_Of_Vector_Leaving_It_Unchanged() {
    V2D diag(1, 1);
    TS_ASSERT_DELTA(diag.norm(), M_SQRT2, DBL_EPSILON);
    TS_ASSERT_EQUALS(diag.X(), 1.0);
    TS_ASSERT_EQUALS(diag.Y(), 1.0);
  }

  void test_Norm2_Gives_Length_Squared_Of_Vector_Leaving_It_Unchanged() {
    V2D diag(2, 2);
    TS_ASSERT_DELTA(diag.norm2(), 8.0, DBL_EPSILON);
    TS_ASSERT_EQUALS(diag.X(), 2.0);
    TS_ASSERT_EQUALS(diag.Y(), 2.0);
  }

  void test_Dot_Product_Matches_Expected_Value() {
    V2D first(0., 1.);
    V2D second(1., 1.);
    double value = first.scalar_prod(second);
    TS_ASSERT_DELTA(value, 1.0, DBL_EPSILON);
    // symmetric
    TS_ASSERT_DELTA(second.scalar_prod(first), value, DBL_EPSILON);

    first = V2D(2., 4.);
    second = V2D(6., 8.);
    TS_ASSERT_DELTA(first.scalar_prod(second), 44.0, DBL_EPSILON);
    // Symmetric
    TS_ASSERT_DELTA(second.scalar_prod(first), 44.0, DBL_EPSILON);
  }

  void test_Cross_Product_Gives_3D_Vector_Perpendicular_To_Input() {
    V2D first(1., 0.);
    V2D second(0., 1.);
    V3D cross = first.cross_prod(second);
    TS_ASSERT_EQUALS(cross.X(), 0.0);
    TS_ASSERT_EQUALS(cross.Y(), 0.0);
    TS_ASSERT_EQUALS(cross.Z(), 1.0);
    // Orientation depends on order
    V3D reverse = V3D(-1.0, -1.0, -1.0) * cross;
    TS_ASSERT_EQUALS(second.cross_prod(first), reverse);
  }

  void test_Distance_Between_Two_Points_As_Vectors() {
    V2D first(3.0, 0.0);
    V2D second(3.0, 4.0);
    // Forms a 3-4-5 triangle therefore distance=4 (the vertical drop)
    TS_ASSERT_DELTA(first.distance(second), 4.0, DBL_EPSILON);
    // Symmetric in arguments
    TS_ASSERT_DELTA(second.distance(first), 4.0, DBL_EPSILON);
  }

  void test_Angle_Between_Two_Vectors() {
    V2D first(1.0, 0.0);
    V2D second(1.0, 1.0);
    // Forms a 1-1-Sqrt(2) triangle therefore angle = 45 degrees (pi/4 radians)
    TS_ASSERT_DELTA(first.angle(second), M_PI / 4.0, DBL_EPSILON);
    // Symmetric in arguments
    TS_ASSERT_DELTA(second.angle(first), M_PI / 4.0, DBL_EPSILON);

    // Equilateral so angle = 60 degrees
    first = V2D(1.0, 0.0);
    second = V2D(1.0, std::sqrt(3.0));
    TS_ASSERT_DELTA(first.angle(second), M_PI / 3.0, DBL_EPSILON);
    // Symmetric in arguments
    TS_ASSERT_DELTA(second.angle(first), M_PI / 3.0, DBL_EPSILON);
  }

  void test_Equality_Operator() {

    V2D first(1E-7, 0.1);
    V2D second(1.5E-7, 0.1);

    TS_ASSERT(first != second);
  }
};

#endif // MANTID_KERNEL_V2DTEST_H_

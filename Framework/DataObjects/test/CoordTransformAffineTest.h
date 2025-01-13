// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CoordTransform.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VMD.h"
#include <cxxtest/TestSuite.h>

#include <boost/scoped_ptr.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::API::CoordTransform;

class CoordTransformAffineTest : public CxxTest::TestSuite {

private:
  /** Helper to compare two "vectors" (bare float array and V3D) */
  void compare(size_t numdims, coord_t *value, const Mantid::Kernel::V3D &expected) {
    for (size_t i = 0; i < numdims; i++)
      TS_ASSERT_DELTA(value[i], expected[i], 1e-4);
  }

  /** Helper to compare two "vectors" (bare float arrays) */
  void compare(size_t numdims, coord_t *value, const coord_t *expected) {
    for (size_t i = 0; i < numdims; i++)
      TS_ASSERT_DELTA(value[i], expected[i], 1e-4);
  }

  /** Helper to create a rotation tranformation*/
  Mantid::Kernel::Matrix<coord_t> createRotationTransform(const Mantid::Kernel::V3D &ax, const Mantid::Kernel::V3D &ay,
                                                          const Mantid::Kernel::V3D &az, const Mantid::Kernel::V3D &bx,
                                                          const Mantid::Kernel::V3D &by,
                                                          const Mantid::Kernel::V3D &bz) {
    Mantid::Kernel::Matrix<coord_t> transform(4, 4);
    transform[0][0] = coord_t(ax.scalar_prod(bx));
    transform[0][1] = coord_t(ax.scalar_prod(by));
    transform[0][2] = coord_t(ax.scalar_prod(bz));
    transform[0][3] = 0;
    transform[1][0] = coord_t(ay.scalar_prod(bx));
    transform[1][1] = coord_t(ay.scalar_prod(by));
    transform[1][2] = coord_t(ay.scalar_prod(bz));
    transform[1][3] = 0;
    transform[2][0] = coord_t(az.scalar_prod(bx));
    transform[2][1] = coord_t(az.scalar_prod(by));
    transform[2][2] = coord_t(az.scalar_prod(bz));
    transform[2][3] = 0;
    transform[3][0] = 0;
    transform[3][1] = 0;
    transform[3][2] = 0;
    transform[3][3] = 1;
    return transform;
  }

public:
  void test_initialization() {
    // Can't output more dimensions than the input
    TS_ASSERT_THROWS_ANYTHING(CoordTransformAffine ct_cant(2, 3))
    CoordTransformAffine ct(2, 1);
    TS_ASSERT_EQUALS(ct.getMatrix().numRows(), 2);
    TS_ASSERT_EQUALS(ct.getMatrix().numCols(), 3);
  }

  /** Simple identity transform */
  void test_donothing() {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    CoordTransformAffine ct(2, 2); // defaults to identity
    ct.apply(in, out);
    compare(2, out, in);
  }

  /** Translate in 2D */
  void test_translate() {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    coord_t translation[2] = {2.0, 3.0};
    coord_t expected[2] = {3.5, 5.5};
    CoordTransformAffine ct(2, 2);
    ct.addTranslation(translation);
    ct.apply(in, out);
    compare(2, out, expected);
  }

  /** Clone a translation */
  void test_clone() {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    coord_t translation[2] = {2.0, 3.0};
    coord_t expected[2] = {3.5, 5.5};
    CoordTransformAffine ct(2, 2);
    ct.addTranslation(translation);

    // Clone and check the clone works
    boost::scoped_ptr<CoordTransform> clone(ct.clone());
    clone->apply(in, out);
    compare(2, out, expected);
  }

  /** apply() method with VMD */
  void test_apply_VMD() {
    coord_t translation[2] = {2.0, 3.0};
    CoordTransformAffine ct(2, 2);
    ct.addTranslation(translation);
    // Transform a VMD
    VMD in(1.5, 2.5);
    VMD out = ct.applyVMD(in);
    TS_ASSERT_DELTA(out[0], 3.5, 1e-5);
    TS_ASSERT_DELTA(out[1], 5.5, 1e-5);
    // Wrong number of dimensions?
    TSM_ASSERT_THROWS_ANYTHING("Check for the right # of dimensions", ct.applyVMD(VMD(1.0, 2.0, 3.0)));
  }

  /** Test rotation in isolation */
  void test_rotation() {
    using Mantid::Kernel::V3D;

    CoordTransformAffine ct(3, 3);

    const V3D ax(1, 0, 0);
    const V3D ay(0, 1, 0);
    const V3D az(0, 0, 1);

    // Following denotes 90 degree rotation about z-axis (clockwise)
    const V3D bx(0, -1, 0);
    const V3D by(1, 0, 0);
    const V3D bz(0, 0, 1);

    Mantid::Kernel::Matrix<coord_t> transform = createRotationTransform(ax, ay, az, bx, by, bz);
    ct.setMatrix(transform);

    coord_t out[3];

    coord_t in_ax[3] = {1, 0, 0}; // Vector along x-axis ax
    ct.apply(in_ax, out);
    compare(3, out, bx);

    coord_t in_ay[3] = {0, 1, 0}; // Vector along y-axis ay
    ct.apply(in_ay, out);
    compare(3, out, by);

    coord_t in_az[3] = {0, 0, 1}; // Vector along z-axis az
    ct.apply(in_az, out);
    compare(3, out, az);

    coord_t in_axyz[3] = {1, 1, 1}; // Vector (1 1 1)
    ct.apply(in_axyz, out);
    coord_t expected[3] = {1, -1, 1};
    compare(3, out, expected);
  }

  //-----------------------------------------------------------------------------------------------
  /** Test a case of a rotation 0.1 radians around +Z,
   * and a projection into the XY plane */
  void test_buildOrthogonal() {
    CoordTransformAffine ct(3, 2);

    // Origin is 1.0, 1.0, 1.0
    VMD origin(1.0, 1.0, 1.0);

    double angle = 0.1;
    // Build the basis vectors, a 0.1 rad rotation along +Z
    std::vector<VMD> bases{{cos(angle), sin(angle), 0.0}, {-sin(angle), cos(angle), 0.0}};
    // Scaling is 1.0
    VMD scale(1.0, 1.0);
    // Build it
    TS_ASSERT_THROWS_NOTHING(ct.buildOrthogonal(origin, bases, scale));

    coord_t out[2] = {0, 0};
    // This is the inverse rotation to make points
    Quat q(-0.1 / (M_PI / 180.0), V3D(0, 0, 1));

    // Point is along the X axis
    V3D exp1(0.2, 0.0, 0.0);
    q.rotate(exp1);
    coord_t in1[3] = {1.2f, 1.0, 3.456f};
    ct.apply(in1, out);
    compare(2, out, exp1);

    // Some other random location
    V3D exp2(-2.4, 5.6, 0.0);
    q.rotate(exp2);
    coord_t in2[3] = {-1.4f, 6.6f, 8.987f};
    ct.apply(in2, out);
    compare(2, out, exp2);

    // Checks for failure to build
    // //-----------------------------------------------------------------------------------------------

    bases.emplace_back(1, 2, 3);
    TSM_ASSERT_THROWS_ANYTHING("Too many bases throws", ct.buildOrthogonal(origin, bases, scale));
    bases.resize(2);
    bases[0] = VMD(1, 2, 3, 4);
    TSM_ASSERT_THROWS_ANYTHING("A base has the wrong dimensions", ct.buildOrthogonal(origin, bases, scale));
  }

  //-----------------------------------------------------------------------------------------------
  /** Test a case of a rotation 0.1 radians around +Z,
   * and a projection into the XY plane,
   * and scaling in the output dimensions */
  void test_buildOrthogonal_withScaling() {
    CoordTransformAffine ct(3, 2);

    // Origin is 1.0, 1.0, 1.0
    VMD origin(1.0, 1.0, 1.0);

    double angle = 0.1;
    // Build the basis vectors, a 0.1 rad rotation along +Z
    std::vector<VMD> bases{{cos(angle), sin(angle), 0.0}, {-sin(angle), cos(angle), 0.0}};
    // Scaling
    VMD scale(2.0, 3.0);
    // Build it
    TS_ASSERT_THROWS_NOTHING(ct.buildOrthogonal(origin, bases, scale));

    coord_t out[2] = {0, 0};
    // This is the inverse rotation to make points
    Quat q(-0.1 / (M_PI / 180.0), V3D(0, 0, 1));

    // Some other random location
    V3D exp2(-2.4, 5.6, 0.0);
    q.rotate(exp2);
    coord_t in2[3] = {-1.4f, 6.6f, 8.987f};
    // The output gets scaled like this
    coord_t scaledExp2[2] = {static_cast<coord_t>(exp2[0] * 2.0), static_cast<coord_t>(exp2[1] * 3.0)};
    ct.apply(in2, out);
    compare(2, out, scaledExp2);

    // Checks for failure to build
    scale = VMD(2, 3, 4);
    TSM_ASSERT_THROWS_ANYTHING("Mismatch in scaling vector", ct.buildOrthogonal(origin, bases, scale));
  }

  //-----------------------------------------------------------------------------------------------
  /// Validate Inputs
  void test_combineTransformations_failures() {
    CoordTransformAffine ct33(3, 3);
    CoordTransformAffine ct43(4, 3);
    CoordTransformAffine ct32(3, 2);
    CoordTransformAffine ct42(4, 2);
    TSM_ASSERT_THROWS_ANYTHING("Null input fails.", CoordTransformAffine::combineTransformations(nullptr, nullptr));
    TSM_ASSERT_THROWS_ANYTHING("Null input fails.", CoordTransformAffine::combineTransformations(nullptr, &ct43));
    TSM_ASSERT_THROWS_ANYTHING("Incompatible # of dimensions",
                               CoordTransformAffine::combineTransformations(&ct42, &ct32));
    TSM_ASSERT_THROWS_ANYTHING("Incompatible # of dimensions",
                               CoordTransformAffine::combineTransformations(&ct32, &ct43));
    CoordTransformAffine *ct(nullptr);
    TSM_ASSERT_THROWS_NOTHING("Compatible # of dimensions",
                              ct = CoordTransformAffine::combineTransformations(&ct43, &ct32));
    delete ct;
    coord_t center[3] = {1, 2, 3};
    bool bools[3] = {true, true, true};
    CoordTransformDistance ctd(3, center, bools);
    TSM_ASSERT_THROWS_ANYTHING("Only aligned or affine inputs",
                               CoordTransformAffine::combineTransformations(&ct33, &ctd));
    TSM_ASSERT_THROWS_ANYTHING("Only aligned or affine inputs",
                               CoordTransformAffine::combineTransformations(&ctd, &ct33));
  }

  //-----------------------------------------------------------------------------------------------
  /** Combine two simple translations */
  void test_combineTransformations_translations() {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    coord_t translation1[2] = {2.0, 3.0};
    coord_t translation2[2] = {5.0, 9.0};
    coord_t expected[2] = {8.5, 14.5};
    CoordTransformAffine ct1(2, 2);
    ct1.addTranslation(translation1);
    CoordTransformAffine ct2(2, 2);
    ct2.addTranslation(translation2);
    // Combine them
    boost::scoped_ptr<CoordTransformAffine> combined(CoordTransformAffine::combineTransformations(&ct1, &ct2));
    combined->apply(in, out);
    compare(2, out, expected);
  }

  //-----------------------------------------------------------------------------------------------
  /// Combine two 2D transforms, compare the result
  void do_test_combined(CoordTransform *ct1, CoordTransform *ct2) {
    coord_t in[2] = {1.5, 2.5};
    coord_t out1[2];
    coord_t out2[2];
    coord_t out_combined[2];

    // First, apply the transform individually
    ct1->apply(in, out1);
    ct2->apply(out1, out2);

    // Combine them
    boost::scoped_ptr<CoordTransformAffine> combined(CoordTransformAffine::combineTransformations(ct1, ct2));
    combined->apply(in, out_combined);

    // Applying the combined one = same as each one in sequence
    compare(2, out_combined, out2);
  }

  //-----------------------------------------------------------------------------------------------
  /// Combine two complex affine transforms
  void test_combineTransformations_affine_affine() {
    CoordTransformAffine ct1(2, 2);
    double angle = 0.1;
    std::vector<VMD> bases1{{cos(angle), sin(angle)}, {-sin(angle), cos(angle)}};
    ct1.buildOrthogonal(VMD(3.0, 4.0), bases1, VMD(5.5, -6.7));

    CoordTransformAffine ct2(2, 2);
    angle = +0.34;
    std::vector<VMD> bases2{{cos(angle), sin(angle)}, {-sin(angle), cos(angle)}};
    ct2.buildOrthogonal(VMD(8.0, -9.0), bases2, VMD(0.34, 12.5));
    // And test
    do_test_combined(&ct1, &ct2);
  }

  //-----------------------------------------------------------------------------------------------
  /// Combine affine + aligned
  void test_combineTransformations_affine_aligned() {
    CoordTransformAffine ct1(2, 2);
    double angle = 0.1;
    std::vector<VMD> bases1{{cos(angle), sin(angle)}, {-sin(angle), cos(angle)}};
    ct1.buildOrthogonal(VMD(3.0, 4.0), bases1, VMD(5.5, -6.7));

    size_t dimensionToBinFrom[2] = {1, 0};
    coord_t origin[2] = {-12.5, +34.5};
    coord_t scaling[2] = {-3.5, +2.25};
    CoordTransformAligned ct2(2, 2, dimensionToBinFrom, origin, scaling);

    // And test
    do_test_combined(&ct1, &ct2);
  }

  //-----------------------------------------------------------------------------------------------
  void testSerialization() {
    using Mantid::Kernel::V3D;
    CoordTransformAffine ct(3, 3);

    // Generate a transformation matrix. NB. This is not composed of a well
    // formed transformation or rotation matrix.
    Mantid::Kernel::Matrix<coord_t> transform(4, 4);
    int count = 0;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        transform[i][j] = static_cast<coord_t>(count);
        count++;
      }
    }

    ct.setMatrix(transform);

    std::string expected = std::string("<CoordTransform>") + "<Type>CoordTransformAffine</Type>" + "<ParameterList>" +
                           "<Parameter><Type>InDimParameter</Type><Value>3</Value></Parameter>" +
                           "<Parameter><Type>OutDimParameter</Type><Value>3</Value></Parameter>" +
                           "<Parameter><Type>AffineMatrixParameter</"
                           "Type><Value>0,1,2,3;4,5,6,7;8,9,10,11;12,13,14,15</Value></"
                           "Parameter>" +
                           "</ParameterList>" + "</CoordTransform>";

    std::string res = ct.toXMLString();

    TSM_ASSERT_EQUALS("Serialization of CoordTransformAffine has not worked correctly.", expected, ct.toXMLString());
  }
};

class CoordTransformAffineTestPerformance : public CxxTest::TestSuite {
public:
  void test_apply_3D_performance() {
    // Do a simple 3-3 transform.
    CoordTransformAffine ct(3, 3);
    coord_t translation[3] = {2.0, 3.0, 4.0};
    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out[3];
    ct.addTranslation(translation);

    for (size_t i = 0; i < 1000 * 1000 * 10; ++i) {
      ct.apply(in, out);
    }
  }
  void test_apply_4D_performance() {
    CoordTransformAffine ct(4, 4);
    coord_t translation[4] = {2.0, 3.0, 4.0, 5.0};
    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out[4];
    ct.addTranslation(translation);

    for (size_t i = 0; i < 1000 * 1000 * 10; ++i) {
      ct.apply(in, out);
    }
  }
};

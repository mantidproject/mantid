#ifndef MANTID_MDEVENTS_COORDTRANSFORMAFFINETEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMAFFINETEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDEvents;

class CoordTransformAffineTest : public CxxTest::TestSuite
{

private:

  /** Helper to compare two "vectors" (bare float array and V3D) */
  void compare(size_t numdims, coord_t * value, const Mantid::Kernel::V3D& expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  /** Helper to compare two "vectors" (bare float arrays) */
  void compare(size_t numdims, coord_t * value, const coord_t * expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  /** Helper to create a rotation tranformation*/
  Mantid::Kernel::Matrix<coord_t> createRotationTransform(
    const Mantid::Kernel::V3D& ax,
    const Mantid::Kernel::V3D& ay,
    const Mantid::Kernel::V3D& az,
    const Mantid::Kernel::V3D& bx,
    const Mantid::Kernel::V3D& by,
    const Mantid::Kernel::V3D& bz)
  {
    Mantid::Kernel::Matrix<coord_t> transform(4, 4);
    transform[0][0] = ax.scalar_prod(bx);
    transform[0][1] = ax.scalar_prod(by);
    transform[0][2] = ax.scalar_prod(bz);
    transform[0][3] = 0;
    transform[1][0] = ay.scalar_prod(bx);
    transform[1][1] = ay.scalar_prod(by);
    transform[1][2] = ay.scalar_prod(bz);
    transform[1][3] = 0;
    transform[2][0] = az.scalar_prod(bx);
    transform[2][1] = az.scalar_prod(by);
    transform[2][2] = az.scalar_prod(bz);
    transform[2][3] = 0;
    transform[3][0] = 0;
    transform[3][1] = 0;
    transform[3][2] = 0;
    transform[3][3] = 1;
    return transform;
  }

public:

  void test_initialization()
  {
    // Can't output more dimensions than the input
    TS_ASSERT_THROWS_ANYTHING(CoordTransformAffine ct_cant(2,3))
        CoordTransformAffine ct(2,1);
    TS_ASSERT_EQUALS( ct.getMatrix().numRows(), 2);
    TS_ASSERT_EQUALS( ct.getMatrix().numCols(), 3);
  }

  /** Simple identity transform */
  void test_donothing()
  {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    CoordTransformAffine ct(2,2); // defaults to identity
    ct.apply(in, out);
    compare(2, out, in);
  }

  /** Translate in 2D */
  void test_translate()
  {
    coord_t in[2] = {1.5, 2.5};
    coord_t out[2];
    coord_t translation[2] = {2.0, 3.0};
    coord_t expected[2] = {3.5, 5.5};
    CoordTransformAffine ct(2,2);
    ct.addTranslation(translation);
    ct.apply(in, out);
    compare(2, out, expected);
  }

  /** Test rotation in isolation */
  void test_rotation()
  {
    using Mantid::Kernel::V3D;

    CoordTransformAffine ct(3, 3);

    const V3D ax(1, 0, 0);
    const V3D ay(0, 1, 0);
    const V3D az(0, 0, 1);

    //Following denotes 90 degree rotation about z-axis (clockwise)
    const V3D bx(0, -1, 0);
    const V3D by(1, 0, 0);
    const V3D bz(0, 0, 1);

    Mantid::Kernel::Matrix<coord_t> transform = createRotationTransform(ax, ay, az, bx, by, bz);
    ct.setMatrix(transform);

    coord_t out[3];

    coord_t in_ax[3] = {1, 0, 0};  //Vector along x-axis ax
    ct.apply(in_ax, out);
    compare(3, out, bx);

    coord_t in_ay[3] = {0, 1, 0};  //Vector along y-axis ay
    ct.apply(in_ay, out);
    compare(3, out, by);

    coord_t in_az[3] = {0, 0, 1};  //Vector along z-axis az
    ct.apply(in_az, out);
    compare(3, out, az);

    coord_t in_axyz[3] = {1, 1, 1};  //Vector (1 1 1)
    ct.apply(in_axyz, out);
    coord_t expected[3] = {1, -1, 1};
    compare(3, out, expected);
  }

  void testSerialization()
  {
    using Mantid::Kernel::V3D;
    CoordTransformAffine ct(3, 3);

    //Generate a transformation matrix. NB. This is not composed of a well formed transformation or rotation matrix.
    Mantid::Kernel::Matrix<coord_t> transform(4,4);
    int count = 0;
    for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
      {
        transform[i][j] = count;
        count++;
      }
    }

    ct.setMatrix(transform);

    std::string expected = std::string("<CoordTransform>") +
      "<Type>CoordTransformAffine</Type>" +
      "<ParameterList>" +
      "<Parameter><Type>InDimParameter</Type><Value>3</Value></Parameter>" +
      "<Parameter><Type>OutDimParameter</Type><Value>3</Value></Parameter>" +
      "<Parameter><Type>AffineMatrixParameter</Type><Value>0,1,2,3;4,5,6,7;8,9,10,11;12,13,14,15</Value></Parameter>" +
      "</ParameterList>" +
      "</CoordTransform>";

    std::string res = ct.toXMLString();

    TSM_ASSERT_EQUALS("Serialization of CoordTransformAffine has not worked correctly.", expected, ct.toXMLString());
  }

};


class CoordTransformAffineTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    // Do a simple 3-3 transform.
    CoordTransformAffine ct(3,3);
    coord_t translation[3] = {2.0, 3.0, 4.0};
    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out[3];
    ct.addTranslation(translation);

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }
  void test_apply_4D_performance()
  {
    CoordTransformAffine ct(4,4);
    coord_t translation[4] = {2.0, 3.0, 4.0, 5.0};
    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out[4];
    ct.addTranslation(translation);

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }

};



#endif /* MANTID_MDEVENTS_COORDTRANSFORMAFFINETEST_H_ */


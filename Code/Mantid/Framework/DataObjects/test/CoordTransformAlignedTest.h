#ifndef MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidKernel/Matrix.h"
#include "MantidMDEvents/CoordTransformAffine.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class CoordTransformAlignedTest : public CxxTest::TestSuite
{
public:

  void test_constructor_throws()
  {
    TSM_ASSERT_THROWS_ANYTHING( "Bad number of dimensions", CoordTransformAligned ct(0,0, NULL, NULL, NULL); );
    TSM_ASSERT_THROWS_ANYTHING( "Too many output dimensions", CoordTransformAligned ct(3,4, NULL, NULL, NULL); );
    TSM_ASSERT_THROWS_ANYTHING( "Null input", CoordTransformAligned ct(1,1, NULL, NULL, NULL); );
    size_t dimToBinFrom[3] = {4, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    TSM_ASSERT_THROWS_ANYTHING( "DimtoBinFrom has too high an index", CoordTransformAligned ct(4,3, dimToBinFrom, origin, scaling); );
    std::vector<size_t> d(3);
    std::vector<coord_t> o(2), s(3);
    TSM_ASSERT_THROWS_ANYTHING( "Non-matching vector lengths", CoordTransformAligned ct(3,3, d,o,s); );
  }

  /** Construct from vector */
  void test_constructor_vector_and_apply()
  {
    std::vector<size_t> dimToBinFrom(3, 0);
    dimToBinFrom[1] = 1; dimToBinFrom[2] = 2;
    std::vector<coord_t> origin(3, 1);
    std::vector<coord_t> scaling(3, 2);
    CoordTransformAligned ct(3,3, dimToBinFrom, origin, scaling);

    coord_t input[3] = {2, 3, 4};
    coord_t output[3] = {0,0,0};
    ct.apply(input, output);
    TS_ASSERT_DELTA( output[0], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 4.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 6.0, 1e-6 );
  }


  void test_constructor_and_apply()
  {
    size_t dimToBinFrom[3] = {3, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned ct(4,3, dimToBinFrom, origin, scaling);

    coord_t input[4] = {16, 11, 11111111 /*ignored*/, 6};
    coord_t output[3] = {0,0,0};
    ct.apply(input, output);
    TS_ASSERT_DELTA( output[0], 1.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 3.0, 1e-6 );
  }

  /// Clone the transform, check that it still works
  void test_clone()
  {
    size_t dimToBinFrom[3] = {3, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned ct(4,3, dimToBinFrom, origin, scaling);
    CoordTransform * clone = ct.clone();

    coord_t input[4] = {16, 11, 11111111 /*ignored*/, 6};
    coord_t output[3] = {0,0,0};
    clone->apply(input, output);
    TS_ASSERT_DELTA( output[0], 1.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 3.0, 1e-6 );
  }

  /// Turn the aligned transform into an affine transform
  void test_makeAffineMatrix()
  {
    size_t dimToBinFrom[3] = {3, 1, 0};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned cto(4,3, dimToBinFrom, origin, scaling);

    Matrix<coord_t> mat = cto.makeAffineMatrix();
    CoordTransformAffine ct(4,3);
    ct.setMatrix(mat);
    // Test in the same way
    coord_t input[4] = {16, 11, 11111111 /*ignored*/, 6};
    coord_t output[3] = {0,0,0};
    ct.apply(input, output);
    TS_ASSERT_DELTA( output[0], 1.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 3.0, 1e-6 );
  }

  /// Turn the aligned transform into an affine transform
  void test_makeAffineMatrix_2()
  {
    size_t dimToBinFrom[3] = {1, 2, 0};
    coord_t origin[3] = {1, 2, 3};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned cto(3,3, dimToBinFrom, origin, scaling);

    // Do the transform direction
    coord_t input[3] = {2, 3, 4};
    coord_t output[3] = {0,0,0};
    cto.apply(input, output);
    TS_ASSERT_DELTA( output[0], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 4.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], -3.0, 1e-6 );

    Matrix<coord_t> mat = cto.makeAffineMatrix();
    CoordTransformAffine ct(3,3);
    ct.setMatrix(mat);

    // Test in the same way
    ct.apply(input, output);
    TS_ASSERT_DELTA( output[0], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 4.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], -3.0, 1e-6 );

    // Do the inverted conversion
    coord_t input2[3] = {2,4,-3};
    //std::cout << "Original\n" << mat << std::endl;
    mat.Invert();
    //std::cout << "Inverted\n" << mat << std::endl;
    ct.setMatrix(mat);
    ct.apply(input2, output);
    TS_ASSERT_DELTA( output[0], 2.0, 1e-6 );
    TS_ASSERT_DELTA( output[1], 3.0, 1e-6 );
    TS_ASSERT_DELTA( output[2], 4.0, 1e-6 );
  }


};


class CoordTransformAlignedTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    // Do a simple 3-3 transform.
    size_t dimToBinFrom[3] = {0, 1, 2};
    coord_t origin[3] = {5, 10, 15};
    coord_t scaling[3] = {1, 2, 3};
    CoordTransformAligned ct(3,3, dimToBinFrom, origin, scaling);

    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out[3];

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }
  void test_apply_4D_performance()
  {
    // Do a simple 4-4 transform.
    size_t dimToBinFrom[4] = {0, 1, 2, 3};
    coord_t origin[4] = {5, 10, 15, 20};
    coord_t scaling[4] = {1, 2, 3, 4};
    CoordTransformAligned ct(4,4, dimToBinFrom, origin, scaling);

    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out[4];

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, out);
    }
  }

};
#endif /* MANTID_MDEVENTS_COORDTRANSFORMALIGNEDTEST_H_ */


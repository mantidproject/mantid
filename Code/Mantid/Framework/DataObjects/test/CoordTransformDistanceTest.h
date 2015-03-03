#ifndef MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_
#define MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidAPI/CoordTransform.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using Mantid::API::CoordTransform;

class CoordTransformDistanceTest : public CxxTest::TestSuite
{
public:

  /** Helper to compare two "vectors" (bare float arrays) */
  void compare(size_t numdims, coord_t * value, const coord_t * expected)
  {
    for (size_t i=0; i< numdims; i++)
      TS_ASSERT_DELTA( value[i], expected[i], 1e-5);
  }

  void test_constructor()
  {
    coord_t center[4] = {1,2,3,4};
    bool used[4] = {true, false, true, true};
    CoordTransformDistance ct(4, center, used);
    // A copy was made
    TS_ASSERT_DIFFERS(ct.getCenter(), center);
    TS_ASSERT_DIFFERS(ct.getDimensionsUsed(), used);

    // Contents are good
    compare(4, center, ct.getCenter());
    for (size_t i=0; i<4; i++)
      TS_ASSERT_EQUALS( used[i], ct.getDimensionsUsed()[i]);
  }


  /** Clone then apply */
  void test_clone()
  {
    // Build it
    coord_t center[2] = {1, 2};
    bool used[2] = {true, true};
    CoordTransformDistance ct(2,center,used);

    CoordTransform * clone = ct.clone();
    coord_t out = 0;
    coord_t in1[2] = {0, 3};
    TS_ASSERT_THROWS_NOTHING( clone->apply(in1, &out) );
    TS_ASSERT_DELTA( out, 2.0, 1e-5);
  }


  /** Calculate the distance (squared)*/
  void test_distance_all_used()
  {
    // Build it
    coord_t center[2] = {1, 2};
    bool used[2] = {true, true};
    CoordTransformDistance ct(2,center,used);

    coord_t out = 0;

    coord_t in1[2] = {0, 3};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in1, &out) );
    TS_ASSERT_DELTA( out, 2.0, 1e-5);

    coord_t in2[2] = {-1, 5};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in2, &out) );
    TS_ASSERT_DELTA( out, 13.0, 1e-5);
  }

  /** Calculate the distance (squared)*/
  void test_distance_some_unused()
  {
    // Build it
    coord_t center[2] = {1, 2};
    bool used[2] = {true, false};
    CoordTransformDistance ct(2,center,used);

    coord_t out = 0;

    coord_t in1[2] = {0, 3};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in1, &out) );
    TS_ASSERT_DELTA( out, 1.0, 1e-5);

    coord_t in2[2] = {-1, 5};
    TS_ASSERT_THROWS_NOTHING( ct.apply(in2, &out) );
    TS_ASSERT_DELTA( out, 4.0, 1e-5);
  }

  /** Test serialization */
  void test_to_xml_string()
  {
    std::string expectedResult = std::string ("<CoordTransform>") +
      "<Type>CoordTransformDistance</Type>" +
      "<ParameterList>" +
      "<Parameter><Type>InDimParameter</Type><Value>4</Value></Parameter>" +
      "<Parameter><Type>OutDimParameter</Type><Value>1</Value></Parameter>" +
      "<Parameter><Type>CoordCenterVectorParam</Type><Value>1.0000,2.0000,2.0000,1.0000</Value></Parameter>" +
      "<Parameter><Type>DimensionsUsedVectorParam</Type><Value>1,0,0,1</Value></Parameter>"
      "</ParameterList>"
      "</CoordTransform>";

    coord_t center[4] = {1, 2, 2, 1};
    bool used[4] = {true, false, false, true};
    CoordTransformDistance ct(4,center,used);
    TSM_ASSERT_EQUALS("CoordTransformDistance has failed to serialize correctly.", expectedResult, ct.toXMLString());
  }

};




class CoordTransformDistanceTestPerformance : public CxxTest::TestSuite
{
public:
  void test_apply_3D_performance()
  {
    coord_t center[3] = {2.0, 3.0, 4.0};
    bool used[3] = {true, true, true};
    CoordTransformDistance ct(3,center,used);
    coord_t in[3] = {1.5, 2.5, 3.5};
    coord_t out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*3, 1e-5);
  }

  void test_apply_4D_performance()
  {
    coord_t center[4] = {2.0, 3.0, 4.0, 5.0};
    bool used[4] = {true, true, true, true};
    CoordTransformDistance ct(4,center,used);
    coord_t in[4] = {1.5, 2.5, 3.5, 4.5};
    coord_t out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*4, 1e-5);
  }

  void test_apply_10D_with_3D_used_performance()
  {
    coord_t center[10] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    bool used[10] = {true, true, true, false, false, false, false, false, false, false};
    CoordTransformDistance ct(10, center,used);
    coord_t in[10] = {1.5, 2.5, 3.5, 4.5, 16, 17, 18, 19, 20, 21};
    coord_t out;

    for (size_t i=0; i<1000*1000*10; ++i)
    {
      ct.apply(in, &out);
    }
    TS_ASSERT_DELTA( out, .25*3, 1e-5);
  }


};


#endif /* MANTID_MDEVENTS_COORDTRANSFORMDISTANCETEST_H_ */


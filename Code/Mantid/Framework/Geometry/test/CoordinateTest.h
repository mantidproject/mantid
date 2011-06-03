#ifndef MANTID_GEOMETRY_CoordinateTEST_H_
#define MANTID_GEOMETRY_CoordinateTEST_H_

#include "MantidGeometry/MDGeometry/Coordinate.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::Geometry;
using namespace Mantid;

class CoordinateTest : public CxxTest::TestSuite
{
public:

  void test_constructor1()
  {
    Coordinate c = Coordinate::createCoordinate1D(1);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 0);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor2()
  {
    Coordinate c = Coordinate::createCoordinate2D(1,2);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor3()
  {
    Coordinate c = Coordinate::createCoordinate3D(1,2,3);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor4()
  {
    Coordinate c = Coordinate::createCoordinate4D(1,2,3,4);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_copy_constructor()
  {
    Coordinate c0 = Coordinate::createCoordinate4D(1,2,3,4);
    Coordinate c(c0);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_constructFromArray4()
  {
    coord_t coords[4] = {1,2,3,4};
    Coordinate c(coords, 4);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_constructFromArray1()
  {
    coord_t coords[1] = {1};
    Coordinate c(coords, 1);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 0);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

};


#endif /* MANTID_GEOMETRY_CoordinateTEST_H_ */


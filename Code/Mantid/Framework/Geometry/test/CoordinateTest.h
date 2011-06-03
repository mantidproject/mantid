#ifndef MANTID_GEOMETRY_COORDINATETEST_H_
#define MANTID_GEOMETRY_COORDINATETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/MDGeometry/Coordinate.h"

using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

class CoordinateTest : public CxxTest::TestSuite
{
public:

  void test_constructor1()
  {
    coordinate c = coordinate::createCoordinate1D(1);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 0);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor2()
  {
    coordinate c = coordinate::createCoordinate2D(1,2);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor3()
  {
    coordinate c = coordinate::createCoordinate3D(1,2,3);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

  void test_constructor4()
  {
    coordinate c = coordinate::createCoordinate4D(1,2,3,4);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_copy_constructor()
  {
    coordinate c0 = coordinate::createCoordinate4D(1,2,3,4);
    coordinate c(c0);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_constructFromArray4()
  {
    coord_t coords[4] = {1,2,3,4};
    coordinate c(coords, 4);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 2);
    TS_ASSERT_EQUALS( c.getZ(), 3);
    TS_ASSERT_EQUALS( c.gett(), 4);
  }

  void test_constructFromArray1()
  {
    coord_t coords[1] = {1};
    coordinate c(coords, 1);
    TS_ASSERT_EQUALS( c.getX(), 1);
    TS_ASSERT_EQUALS( c.getY(), 0);
    TS_ASSERT_EQUALS( c.getZ(), 0);
    TS_ASSERT_EQUALS( c.gett(), 0);
  }

};


#endif /* MANTID_GEOMETRY_COORDINATETEST_H_ */


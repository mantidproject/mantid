#ifndef MANTID_GEOMETRY_V3RTEST_H_
#define MANTID_GEOMETRY_V3RTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/V3R.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class V3RTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static V3RTest *createSuite() { return new V3RTest(); }
  static void destroySuite( V3RTest *suite ) { delete suite; }


  void testConstructors()
  {
      // default constructor
      V3R defConstr;
      TS_ASSERT_EQUALS(defConstr.x(), 0);
      TS_ASSERT_EQUALS(defConstr.y(), 0);
      TS_ASSERT_EQUALS(defConstr.z(), 0);

      // Constructor from rational numbers
      V3R rational(RationalNumber(1, 4), RationalNumber(1, 2), RationalNumber(2, 3));
      V3D rationalV3D = rational;
      TS_ASSERT_EQUALS(rationalV3D.X(), 0.25);
      TS_ASSERT_EQUALS(rationalV3D.Y(), 0.5);
      TS_ASSERT_EQUALS(rationalV3D.Z(), 2.0/3.0);

      // copy constructor
      V3R copied(rational);
      TS_ASSERT_EQUALS(copied.x(), rational.x());
      TS_ASSERT_EQUALS(copied.y(), rational.y());
      TS_ASSERT_EQUALS(copied.z(), rational.z());
  }

  void testXGetterSetter()
  {
      V3R vector;
      TS_ASSERT_EQUALS(vector.x(), 0);

      TS_ASSERT_THROWS_NOTHING(vector.setX(RationalNumber(1, 4)));
      TS_ASSERT_EQUALS(vector.x(), RationalNumber(1, 4));
  }

  void testYGetterSetter()
  {
      V3R vector;
      TS_ASSERT_EQUALS(vector.y(), 0);

      TS_ASSERT_THROWS_NOTHING(vector.setY(RationalNumber(1, 4)));
      TS_ASSERT_EQUALS(vector.y(), RationalNumber(1, 4));
  }

  void testZGetterSetter()
  {
      V3R vector;
      TS_ASSERT_EQUALS(vector.z(), 0);

      TS_ASSERT_THROWS_NOTHING(vector.setZ(RationalNumber(1, 4)));
      TS_ASSERT_EQUALS(vector.z(), RationalNumber(1, 4));
  }

  void testArrayAccess()
  {
      V3R vector(1, 2, 3);
      TS_ASSERT_EQUALS(vector[0], 1);
      TS_ASSERT_EQUALS(vector[1], 2);
      TS_ASSERT_EQUALS(vector[2], 3);
      TS_ASSERT_THROWS(vector[3], Mantid::Kernel::Exception::IndexError);

      TS_ASSERT_THROWS_NOTHING(vector[0] = RationalNumber(2, 3));
      TS_ASSERT_THROWS_NOTHING(vector[1] = RationalNumber(2, 3));
      TS_ASSERT_THROWS_NOTHING(vector[2] = RationalNumber(2, 3));
      TS_ASSERT_THROWS(vector[3] = RationalNumber(2, 3), Mantid::Kernel::Exception::IndexError);
  }


};


#endif /* MANTID_GEOMETRY_V3RTEST_H_ */

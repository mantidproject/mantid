#ifndef MANTID_MDGEOMETRY_MDIMPLICITFUNCTIONTEST_H_
#define MANTID_MDGEOMETRY_MDIMPLICITFUNCTIONTEST_H_

#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid;

class MDPlaneImplicitFunctionTest : public CxxTest::TestSuite {
public:
  void test_addPlane() {
    MDPlaneImplicitFunction f;

    coord_t normal[3] = {1234, 456, 678};
    coord_t point[3] = {1, 2, 3};
    MDPlane p1(3, normal, point);
    MDPlane p2(3, normal, point);

    TS_ASSERT_EQUALS(f.getNumDims(), 0);
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p1));
    TS_ASSERT_EQUALS(f.getNumDims(), 3);
    TS_ASSERT_THROWS_ANYTHING(f.addPlane(p2));
    TS_ASSERT_EQUALS(f.getNumPlanes(), 1);
  }

  void test_coordConstructor() {
    coord_t normal[3] = {1234, 456, 678};
    coord_t point[3] = {1, 2, 3};
    MDPlaneImplicitFunction f(3, normal, point);
    TS_ASSERT_EQUALS(f.getNumDims(), 3);
    // Making sure only one plane can be added
    MDPlane p1(3, normal, point);
    TS_ASSERT_THROWS_ANYTHING(f.addPlane(p1));
  }

  void test_xmlRep() {
    coord_t normal[3] = {1.25, 4.5, 6.75};
    coord_t point[3] = {1, 2, 3};
    MDPlaneImplicitFunction f(3, normal, point);
    TS_ASSERT_EQUALS(f.toXMLString(), getXmlRep());
  }

  void test_xmlRep_addPlane() {
    MDPlaneImplicitFunction f;

    coord_t normal[3] = {1.25, 4.5, 6.75};
    coord_t point[3] = {1, 2, 3};
    MDPlane p1(3, normal, point);
    f.addPlane(p1);
    TS_ASSERT_EQUALS(f.toXMLString(), getXmlRep_noOrigin());
  }

private:
  std::string getXmlRep() {
    return std::string("<Function>"
                       "<Type>PlaneImplicitFuction</Type>"
                       "<ParameterList>"
                       "<Parameter>"
                       "<Type>NormalParameter</Type>"
                       "<Value>1.25 4.5 6.75</Value>"
                       "</Parameter>"
                       "<Parameter>"
                       "<Type>OriginParameter</Type>"
                       "<Value>1 2 3</Value>"
                       "</Parameter>"
                       "</ParameterList>"
                       "</Function>");
  }

  std::string getXmlRep_noOrigin() {
    return std::string("<Function>"
                       "<Type>PlaneImplicitFuction</Type>"
                       "<ParameterList>"
                       "<Parameter>"
                       "<Type>NormalParameter</Type>"
                       "<Value>1.25 4.5 6.75</Value>"
                       "</Parameter>"
                       "<Parameter>"
                       "<Type>OriginParameter</Type>"
                       "<Value>nan nan nan</Value>"
                       "</Parameter>"
                       "</ParameterList>"
                       "</Function>");
  }
};

#endif // MANTID_MDGEOMETRY_MDIMPLICITFUNCTIONTEST_H_

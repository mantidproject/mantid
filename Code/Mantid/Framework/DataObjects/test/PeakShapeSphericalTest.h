#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_

#include <cxxtest/TestSuite.h>
#include <jsoncpp/json/json.h>

#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/SpecialCoordinateSystem.h"

using Mantid::DataObjects::PeakShapeSpherical;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class PeakShapeSphericalTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeSphericalTest *createSuite() { return new PeakShapeSphericalTest(); }
  static void destroySuite( PeakShapeSphericalTest *suite ) { delete suite; }


  void test_constructor()
  {
    const V3D centre(1,1,1);
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(centre, radius, frame, algorithmName, algorithmVersion);

    TS_ASSERT_EQUALS( centre.toString(), shape.centre().toString() );
    TS_ASSERT_EQUALS( radius, shape.radius() );
    TS_ASSERT_EQUALS( frame, shape.frame() );
    TS_ASSERT_EQUALS( algorithmName, shape.algorithmName() );
    TS_ASSERT_EQUALS( algorithmVersion, shape.algorithmVersion() );
  }

  void test_copy_constructor()
  {
    const V3D centre(1,1,1);
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(centre, radius, frame, algorithmName, algorithmVersion);
    // Copy construct it
    PeakShapeSpherical b(a);

    TS_ASSERT_EQUALS( centre.toString(), b.centre().toString() );
    TS_ASSERT_EQUALS( radius, b.radius() );
    TS_ASSERT_EQUALS( frame, b.frame() );
    TS_ASSERT_EQUALS( algorithmName, b.algorithmName() );
    TS_ASSERT_EQUALS( algorithmVersion, b.algorithmVersion() );
  }

  void test_assignment()
  {
    const V3D centre(1,1,1);
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(centre, radius, frame, algorithmName, algorithmVersion);
    PeakShapeSpherical b(V3D(0,0,0), 1.0, QSample, "bar", -2);

    // Assign to it
    b = a;

    // Test the assignments
    TS_ASSERT_EQUALS( a.centre().toString(), b.centre().toString() );
    TS_ASSERT_EQUALS( a.radius(), b.radius() );
    TS_ASSERT_EQUALS( a.frame(), b.frame() );
    TS_ASSERT_EQUALS( a.algorithmName(), b.algorithmName() );
    TS_ASSERT_EQUALS( a.algorithmVersion(), b.algorithmVersion() );
  }

  void test_clone()
  {
      const V3D centre(1,1,1);
      const double radius = 2;
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Construct it.
      PeakShapeSpherical a(centre, radius, frame, algorithmName, algorithmVersion);
      PeakShapeSpherical* clone = a.clone();

      TS_ASSERT_EQUALS( a.centre().toString(), clone->centre().toString() );
      TS_ASSERT_EQUALS( a.radius(), clone->radius() );
      TS_ASSERT_EQUALS( a.frame(), clone->frame() );
      TS_ASSERT_EQUALS( a.algorithmName(), clone->algorithmName() );
      TS_ASSERT_EQUALS( a.algorithmVersion(), clone->algorithmVersion() );
      TS_ASSERT_DIFFERS(clone, &a);
  }

  void test_toJSON()
  {
      const V3D centre(1,1,1);
      const double radius = 2;
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Construct it.
      PeakShapeSpherical shape(centre, radius, frame, algorithmName, algorithmVersion);
      const std::string json = shape.toJSON();

      Json::Reader reader;
      Json::Value output;
      TSM_ASSERT("Should parse as JSON", reader.parse(json, output));

      TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
      TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
      TS_ASSERT_EQUALS(frame, output["frame"].asInt());
      TS_ASSERT_EQUALS(radius, output["radius"].asDouble());
 }

 void test_equals()
 {
     TS_ASSERT_EQUALS(PeakShapeSpherical(V3D(0,0,0), 1.0, QSample), PeakShapeSpherical(V3D(0,0,0), 1.0, QSample));

     TSM_ASSERT_DIFFERS("Different centre", PeakShapeSpherical(V3D(0,0,0), 1.0, QSample), PeakShapeSpherical(V3D(1,0,0), 1.0, QSample));

     TSM_ASSERT_DIFFERS("Different radius", PeakShapeSpherical(V3D(0,0,0), 1.0, QSample), PeakShapeSpherical(V3D(0,0,0), 2.0, QSample));

     TSM_ASSERT_DIFFERS("Different frame", PeakShapeSpherical(V3D(0,0,0), 1.0, QSample), PeakShapeSpherical(V3D(0,0,0), 1.0, QLab));
 }


};


#endif /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_ */

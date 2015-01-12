#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/VMD.h"
#include "MantidAPI/SpecialCoordinateSystem.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class PeakShapeSphericalFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeSphericalFactoryTest *createSuite() { return new PeakShapeSphericalFactoryTest(); }
  static void destroySuite( PeakShapeSphericalFactoryTest *suite ) { delete suite; }


  // More tests needed TODO

  void test_create()
  {
      const V3D centre(1,1,1);
      const double radius = 2;
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Make a source shape
      PeakShapeSpherical sourceShape(centre, radius, frame, algorithmName, algorithmVersion);

      PeakShapeSphericalFactory factory;
      PeakShape* productShape = factory.create(sourceShape.toJSON());

      PeakShapeSpherical* sphericalShapeProduct = dynamic_cast<PeakShapeSpherical*>(productShape);
      TS_ASSERT(sphericalShapeProduct);

      TS_ASSERT_EQUALS(sourceShape, *sphericalShapeProduct);
  }


};


#endif /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_ */

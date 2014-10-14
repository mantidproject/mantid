#ifndef TESTSAMPLEENVIRONMENT_H_
#define TESTSAMPLEENVIRONMENT_H_

#include "MantidAPI/SampleEnvironment.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>


using Mantid::API::SampleEnvironment;

class SampleEnvironmentTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentTest *createSuite() { return new SampleEnvironmentTest(); }
  static void destroySuite( SampleEnvironmentTest *suite ) { delete suite; }

  void testThatConstructorGivingNameCreatesTheCorrectName()
  {
    SampleEnvironment kit("TestKit");
    TS_ASSERT_EQUALS(kit.name(), "TestKit");
  }

  void testAddingElementIncreasesSizeByOne()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;
    SampleEnvironment kit("TestKit");

    TS_ASSERT_EQUALS(0, kit.nelements());
    auto shape = ComponentCreationHelper::createSphere(1.0);
    TS_ASSERT_THROWS_NOTHING(kit.add(*shape));
    TS_ASSERT_EQUALS(1, kit.nelements());

    TS_ASSERT_THROWS_NOTHING(kit.add(*shape));
    TS_ASSERT_EQUALS(2, kit.nelements());
  }

  void testIsValidTestsAllElements()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();

    V3D pt(0.1,0.0,0.0); //inside first, outside second
    TS_ASSERT(kit->isValid(pt));
    pt = V3D(0.3,0.0,0.0); //outside first, inside second
    TS_ASSERT(kit->isValid(pt));
  }

  void testTrackIntersectionTestsAllElements()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();

    Track ray(V3D(), V3D(1.0,0.0,0.0));
    TS_ASSERT_THROWS_NOTHING(kit->interceptSurfaces(ray));
    TS_ASSERT_EQUALS(2, ray.count());
  }

  void testBoundingBoxEncompassesWholeObject()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    auto bbox = kit->boundingBox();

    auto widths = bbox.width();
    TS_ASSERT_DELTA(0.45, widths.X(), 1e-12);
    TS_ASSERT_DELTA(0.2, widths.Y(), 1e-12);
    TS_ASSERT_DELTA(0.2, widths.Z(), 1e-12);
  }

private:

  boost::shared_ptr<SampleEnvironment> createTestKit()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = boost::make_shared<SampleEnvironment>("TestKit");
    kit->add(*ComponentCreationHelper::createSphere(0.1)); //origin
    kit->add(*ComponentCreationHelper::createSphere(0.1, V3D(0.25,0.0,0.0))); //shifted in +x

    return kit;
  }
};



#endif // TESTSAMPLEENVIRONMENT_H_

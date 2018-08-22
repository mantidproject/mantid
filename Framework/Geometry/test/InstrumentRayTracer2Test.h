#ifndef INSTRUMENTRAYTRACER2TEST_H_
#define INSTRUMENTRAYTRACER2TEST_H_

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer2.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
namespace IRT2 = Mantid::Geometry::InstrumentRayTracer2;
using Links = Track::LType;

class InstrumentRayTracer2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentRayTracer2Test *createSuite() {
    return new InstrumentRayTracer2Test();
  }
  static void destroySuite(InstrumentRayTracer2Test *suite) { delete suite; }

  InstrumentRayTracer2Test() {
    // Start logging framework
    Mantid::Kernel::ConfigService::Instance();
  }

  void
  test_That_A_Ray_Which_Just_Intersects_One_Component_Gives_This_Component_Only() {
    create_instrument_and_componentInfo();

    V3D testDir(0.010, 0.0, 15.004);

    Links results = IRT2::traceFromSource(testDir, *m_compInfo);

    TS_ASSERT_EQUALS(results.size(), 1);
  }

private:
  /**
   * Helper methods for tests
   */

  // Creates the objects to use in the tests
  void create_instrument_and_componentInfo() {
    // Create 9 cylindrical detectors and set the instrument
    m_testInstrument =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);

    // Create an instrument visitor
    InstrumentVisitor visitor = (m_testInstrument);

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos = InstrumentVisitor::makeWrappers(*m_testInstrument, nullptr);

    // Unpack the pair
    m_compInfo = std::move(infos.first);
    m_detInfo = std::move(infos.second);
  }

  /**
   * Getters
   */
  Instrument_sptr get_instrument() { return m_testInstrument; }

  Mantid::Geometry::ComponentInfo *get_componentInfo() {
    return m_compInfo.get();
  }

  /**
   * Member variables
   */
  // Holds the Instrument
  Instrument_sptr m_testInstrument;

  // Holds the ComponentInfo
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfo;

  // Holds the DetectorInfo
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfo;
};

#endif /* INSTRUMENTRAYTRACER2TEST_H_ */

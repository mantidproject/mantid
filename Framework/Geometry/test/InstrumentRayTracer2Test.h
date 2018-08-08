#ifndef INSTRUMENTRAYTRACER2TEST_H_
#define INSTRUMENTRAYTRACER2TEST_H_

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer2.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
using Mantid::detid_t;

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

  std::unique_ptr<Mantid::Geometry::ComponentInfo>
  create_component_info_object() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0),   // Source pos
                                           V3D(10, 0, 0),  // Sample pos
                                           V3D(11, 0, 0)); // Detector position

    // Create the instrument visitor
    InstrumentVisitor visitor(visitee);

    // Visit everything
    visitor.walkInstrument();

    // Get the ComponentInfo object
    auto compInfo = InstrumentVisitor::makeWrappers(*visitee, nullptr).first;

    return compInfo;
  }

  void test_get_componentInfo() {
    auto compInfo = create_component_info_object();
    TS_ASSERT(compInfo != nullptr);
  }
};

#endif /* INSTRUMENTRAYTRACER2TEST_H_ */

#ifndef MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::Geometry::ComponentInfoBankHelpers;

class ComponentInfoBankHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoBankHelpersTest *createSuite() {
    return new ComponentInfoBankHelpersTest();
  }
  static void destroySuite(ComponentInfoBankHelpersTest *suite) {
    delete suite;
  }

  void test_DetectorFixedInBankTrueForRectangularBank() {
    auto rectInstr =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*rectInstr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(isDetectorFixedInBank(componentInfo, 0));
  }

  void test_DetectorFixedInBankFalseForNonStructuredBank() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, 0));
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_ */
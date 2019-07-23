#ifndef MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <algorithm>
#include <boost/make_shared.hpp>

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

  void test_DetectorFixedInBankFalseForMonitor() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto det = new Detector("MyTestMonitor", 10, instr.get());
    auto shape = ComponentCreationHelper::createCuboid(0.001, 0.001, 0.001);
    det->setShape(shape);
    det->setPos(V3D(0, 0, 0));
    det->setRot(Quat());

    instr->add(det);
    instr->markAsMonitor(det);

    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    auto index = componentInfo.indexOfAny("MyTestMonitor");

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, index));
  }

  void test_DetectorFixedInBankFalseForNonDetectorComponent() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, componentInfo.root()));
  }

  void test_isAnyBank_false_for_tube() {

    auto instr = ComponentCreationHelper::createInstrumentWithPSDTubes(1, 1);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);

    auto children = compInfo.children(compInfo.root());
    bool tubeInInstrument = std::any_of(
        children.begin(), children.end(), [&compInfo](const size_t &index) {
          return compInfo.componentType(index) ==
                 Beamline::ComponentType::Unstructured;
        });

    // assert at least one tube is in instrument
    TS_ASSERT(tubeInInstrument);
    for (size_t index = compInfo.root() - 1; index > 0; --index) {
      auto compType = compInfo.componentType(index);
      // identify tube and assert isAnyBank returns false
      if (compType == Beamline::ComponentType::Unstructured) {
        TS_ASSERT(!isAnyBank(compInfo, index));
      }
    }
  }

  void test_isAnyBank_false_for_detector() {

    auto instr = ComponentCreationHelper::createMinimalInstrument(
        V3D(0.0, 0.0, -10.0), V3D(0.0, 0.0, 0.0), V3D(0.0, 0.0, 10.0));
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);

    auto children = compInfo.children(compInfo.root());
    bool detInInstrument = std::any_of(children.begin(), children.end(),
                                       [&compInfo](const size_t &index) {
                                         return compInfo.isDetector(index);
                                       });

    // assert at least one detector is in instrument
    TS_ASSERT(detInInstrument);
    for (size_t index = compInfo.root() - 1; index > 0; --index) {
      std::string name = compInfo.name(index);
      // identify tube and assert isAnyBank returns false
      if (compInfo.isDetector(index)) {
        TS_ASSERT(!isAnyBank(compInfo, index));
      }
    }
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_ */

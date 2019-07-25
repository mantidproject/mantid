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

  void test_isAnyBank_false_for_tubes() {

    auto instr = ComponentCreationHelper::createInstrumentWithPSDTubes(2, 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);

    const size_t tubeIdx1 = 5;
    const size_t tubeIdx2 = 4;

    TS_ASSERT(compInfo.componentType(tubeIdx1) ==
              Beamline::ComponentType::OutlineComposite);
    TS_ASSERT(compInfo.componentType(tubeIdx2) ==
              Beamline::ComponentType::OutlineComposite);

    TS_ASSERT(!isAnyBank(compInfo, tubeIdx1));
    TS_ASSERT(!isAnyBank(compInfo, tubeIdx2));
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

  void test_isAnyBank_finds_rectangular() {
    auto instr =
        ComponentCreationHelper::createTestInstrumentRectangular2(2, 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);

    const size_t bankIdx = 13;

    TS_ASSERT(compInfo.componentType(bankIdx) ==
              Beamline::ComponentType::Rectangular); // assert rectangular bank
                                                     // at bankIdx
    TS_ASSERT(isAnyBank(compInfo, bankIdx))
  }

  void test_offsetFromAncestor_gets_expected_offset() {

    const Quat relativeBankRotation(45.0, V3D(0.0, 1.0, 0.0));
    const Quat relativeDetRotation(45.0, V3D(0.0, 1.0, 0.0));
    const V3D absBankposition(0, 0, 10);
    const V3D detectorOffset(2.0, -2.0, 0.0);

    auto instr = ComponentCreationHelper::createSimpleInstrumentWithRotation(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        absBankposition,      // bank position
        relativeBankRotation, // bank rotation
        relativeDetRotation,
        detectorOffset); // detector rotation, detector offset

    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);

    const size_t bankIdx = 3;
    const size_t detIdx = 0;

    TS_ASSERT(compInfo.isDetector(detIdx));  // assert detector at this index
    TS_ASSERT(isAnyBank(compInfo, bankIdx)); // assert bank at this index

    // undo bank rotation
    auto transformation = Eigen::Affine3d(
        Mantid::Kernel::toQuaterniond(compInfo.rotation(bankIdx)).conjugate());

    // undo bank translation
    transformation.translate(
        -Mantid::Kernel::toVector3d(compInfo.position(bankIdx)));

    // get back expected offset
    auto expected_offset =
        transformation * Mantid::Kernel::toVector3d(compInfo.position(detIdx));

    auto detector_offset_from_bank =
        offsetFromAncestor(compInfo, bankIdx, detIdx);

    TS_ASSERT(expected_offset.isApprox(detector_offset_from_bank));
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_ */

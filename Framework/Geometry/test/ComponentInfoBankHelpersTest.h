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

  void test_isSaveableBank_false_for_tubes() {
    // test instrument with detector tubes to test that IsSaveableBank will
    // return false detector tubes
    auto instr = ComponentCreationHelper::createInstrumentWithPSDTubes(2, 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);
    const auto &detInfo = (*wrappers.second);
    const size_t tubeIdx = 5; // index of tube in component info

    // verify component is tube
    TS_ASSERT(compInfo.componentType(tubeIdx) ==
              Beamline::ComponentType::OutlineComposite);
    // assert isSaveableBank returns false
    TS_ASSERT(!isSaveableBank(compInfo, tubeIdx));
  }

  void test_isSaveableBank_false_for_detector() {
    // test instrument with a detetctor to test that IsSaveableBank will
    // return false for detectors
    auto instr = ComponentCreationHelper::createMinimalInstrument(
        V3D(0.0, 0.0, -10.0), V3D(0.0, 0.0, 0.0), V3D(0.0, 0.0, 10.0));
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);
    const auto &detInfo = (*wrappers.second);

    TS_ASSERT(!isSaveableBank(compInfo, 0 /*index of detector*/));
  }

  void test_isSaveableBank_finds_rectangular() {
    // create an instrument with a rectangular detector bank
    auto instr = ComponentCreationHelper::createTestInstrumentRectangular2(
        2 /*number of banks*/, 2 /*number of pixels*/);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);
    const auto &detInfo = (*wrappers.second);
    // index of rectangular bank
    const size_t bankIdx = 13;
    // assert rectangular bank at bankIdx

    TS_ASSERT(compInfo.componentType(bankIdx) ==
              Beamline::ComponentType::Rectangular);
    // assert isSaveableBank returns true
    TS_ASSERT(isSaveableBank(compInfo, bankIdx))
  }

  void test_offsetFromAncestor_gets_expected_offset() {
    /*
     Provide offsetFromAncestor with a bank index as the 'ancestor', and a
     detector as the 'current' index, and assert that offsetFromAncestor returns
     the specified detector offset from the bank.

     Preparation of geometry for test instrument:
     bank position is 10m along z, the detector is then offset from the bank
     with xyz value (2,-2, 0) . The bank is then rotated 45 degrees about y,
     detector in bank is then rotated an additional 45 degrees, therefore the
     detector has a net rotation of 90 degrees. offsetFromAncestor should be
     able to retrieve the detector offset (2,-2,0) relative to the bank by
     internally applying the reverse transformations unto the position of the
     detector.
        */

    // specify the detector offset that offsetFromAncestor should retrieve from
    // compInfo.
    const V3D detectorOffset(2.0, -2.0, 0.0);

    // create instrument with geometry as above
    auto instr = ComponentCreationHelper::createSimpleInstrumentWithRotation(
        Mantid::Kernel::V3D(0.0, 0.0, -10.0) /*arbitrary source pos*/,
        Mantid::Kernel::V3D(0.0, 0.0, 0.0) /*arbitrary sample pos*/,
        V3D(0.0, 0.0, 10.0) /* bank position*/,
        Quat(45.0, V3D(0.0, 1.0, 0.0)) /* bank rotation*/,
        Quat(45.0, V3D(0.0, 1.0, 0.0)) /* detector position*/,
        detectorOffset); // detector offset which is expected back.

    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &compInfo = (*wrappers.first);
    const size_t bankIdx = 3; // bank index
    const size_t detIdx = 0;  // detector index
    // Eigen copy of the detector offset that was specified in the instrument
    Eigen::Vector3d expected_offset =
        Mantid::Kernel::toVector3d(detectorOffset);
    // ofsset returned by offsetFromAncestor
    auto returnedOffset = offsetFromAncestor(compInfo, bankIdx, detIdx);
    // assert offsetFromAncestor gives back the detector offset

    TS_ASSERT(expected_offset.isApprox(returnedOffset));
  }

  void
  test_offsetFromAncestor_throws_if_ancestor_index_is_not_greater_than_current_index() {

    /* provide offsetFromAncestor with an ancestor index value not greater than
    current index, and assert offsetFromAncestor will throw. */

    // test instrument with arbitrary geometry
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10), V3D(0, 0, 0), V3D(0, 0, 10));
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);

    // instrument cache to be used with call offsetFromAncestor
    const auto &compInfo = (*wrappers.first);

    size_t ancestorIndex = 0; // proposed ancestor < current index
    size_t currentIndex = 1;  // proposed current index
    TS_ASSERT_THROWS(offsetFromAncestor(compInfo, ancestorIndex, currentIndex),
                     std::invalid_argument &);
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_ */

#ifndef MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_
#define MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Crystal::CalibrationHelpers;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CalibrationHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalibrationHelpersTest *createSuite() {
    return new CalibrationHelpersTest();
  }
  static void destroySuite(CalibrationHelpersTest *suite) { delete suite; }

  void test_fixUpSampleAndSourcePositions_moves_the_sample_and_the_source() {
    // Create two identical workspaces
    const auto wsOld =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1000,
                                                                     true);
    auto wsNew = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        2, 1000, true);

    const auto &positionSampleOld = wsOld->detectorInfo().samplePosition();
    const auto &positionSourceOld = wsOld->detectorInfo().sourcePosition();

    const auto &instNew = wsNew->getInstrument();
    const double l1 = 10.0;
    const double newZ = 3.0;
    const auto &positionSampleNew = V3D(1.0, 2.0, newZ);
    const auto &positionSourceNew = V3D(1.0, 2.0, newZ - l1);

    // Make a first move

    CalibrationHelpers::fixUpSampleAndSourcePositions(
        instNew, l1, positionSampleNew, wsNew->mutableDetectorInfo());

    // Old workspace has sample unchanged
    TS_ASSERT_EQUALS(wsOld->detectorInfo().samplePosition(), positionSampleOld);
    // New workspace has sample at the new position
    TS_ASSERT_EQUALS(wsNew->detectorInfo().samplePosition(), positionSampleNew);
    // Old workspace has source at the old position
    TS_ASSERT_EQUALS(wsOld->detectorInfo().sourcePosition(), positionSourceOld);
    // New workspace has source at the new position
    TS_ASSERT_EQUALS(wsNew->detectorInfo().sourcePosition(), positionSourceNew);

    // Make a second move - here the old and new workspaces have different
    // positions

    auto wsNew2 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        2, 1000, true);
    const auto &positionSampleNew2 = V3D(-1.0, -2.0, -3.0);
    CalibrationHelpers::fixUpSampleAndSourcePositions(
        instNew, l1, positionSampleNew2, wsNew2->mutableDetectorInfo());

    // Old workspace has sample unchanged
    TS_ASSERT_EQUALS(wsNew->detectorInfo().samplePosition(), positionSampleNew);
    // New workspace has sample at the new position
    TS_ASSERT_EQUALS(wsNew2->detectorInfo().samplePosition(),
                     positionSampleOld);
    // Old workspace has source at the old position
    TS_ASSERT_EQUALS(wsNew->detectorInfo().sourcePosition(), positionSourceNew);
    // New workspace has source at the new position
    TS_ASSERT_EQUALS(wsNew2->detectorInfo().sourcePosition(),
                     positionSourceOld);
  }

  void test_fixUpBankParameterMap_applies_move_to_rectangular_detectors() {
    setUpFixUpBankParameterMap();

    auto &detectorInfoWsNew = wsNew->mutableDetectorInfo();
    const auto &instNew = wsNew->getInstrument();
    const auto &newPos = V3D(1.0, 2.0, 3.0);
    const auto &newRot = Quat(1.0, 0.0, 0.0, 0.0);
    const double heightScale = 1.0;
    const double widthScale = 1.0;
    std::vector<std::string> bankNames = {"bank1", "bank3"};

    CalibrationHelpers::fixUpBankPositionsAndSizes(
        bankNames, instNew, newPos, newRot, heightScale, widthScale, false,
        detectorInfoWsNew);

    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_1),
                     newPos + oldPosFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_2),
                     oldPosFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_3),
                     newPos + oldPosFirstBank3);

    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_1),
                     oldRotFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_2),
                     oldRotFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_3),
                     oldRotFirstBank3);
  }

  void test_fixUpBankParameterMap_applies_scale_to_rectangular_detectors() {
    setUpFixUpBankParameterMap();

    auto &detectorInfoWsNew = wsNew->mutableDetectorInfo();
    const auto &instNew = wsNew->getInstrument();
    const auto &newPos = V3D(0.0, 0.0, 0.0);
    const auto &newRot = Quat(1.0, 0.0, 0.0, 0.0);
    const double heightScale = 2.0;
    const double widthScale = 3.0;

    CalibrationHelpers::fixUpBankPositionsAndSizes(
        bankNames, instNew, newPos, newRot, heightScale, widthScale, false,
        detectorInfoWsNew);

    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_1).X(),
                     heightScale * oldPosLastBank1.X());
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_1).Y(),
                     widthScale * oldPosLastBank1.Y());
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_1).Z(),
                     oldPosLastBank1.Z());
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_2),
                     oldPosLastBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_3).X(),
                     heightScale * oldPosLastBank3.X());
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_3).Y(),
                     widthScale * oldPosLastBank3.Y());
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_3).Z(),
                     oldPosLastBank3.Z());
  }

  void test_fixUpBankParameterMap_applies_rotation_to_rectangular_detectors() {
    setUpFixUpBankParameterMap();

    auto &detectorInfoWsNew = wsNew->mutableDetectorInfo();
    const auto &instNew = wsNew->getInstrument();
    const auto &newPos = V3D(0.0, 0.0, 0.0);
    const auto &newRot = Quat(0.2, 0.2, 0.2, 0.2);
    const double heightScale = 1.0;
    const double widthScale = 1.0;

    CalibrationHelpers::fixUpBankPositionsAndSizes(
        bankNames, instNew, newPos, newRot, heightScale, widthScale, false,
        detectorInfoWsNew);

    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_1),
                     oldPosFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_2),
                     oldPosFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_3),
                     oldPosFirstBank3);

    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_1),
                     newRot * oldRotFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_2),
                     oldRotFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_3),
                     newRot * oldRotFirstBank3);
  }

  void
  test_fixUpBankParameterMap_applies_all_changes_to_rectangular_detectors() {
    setUpFixUpBankParameterMap();

    auto &detectorInfoWsNew = wsNew->mutableDetectorInfo();
    const auto &instNew = wsNew->getInstrument();
    const auto &newPos = V3D(1.0, 2.0, 3.0);
    const auto &newRot = Quat(0.2, 0.2, 0.2, 0.2);
    const double heightScale = 2.0;
    const double widthScale = 3.0;

    std::vector<std::string> bankNames = {"bank1", "bank3"};

    CalibrationHelpers::fixUpBankPositionsAndSizes(
        bankNames, instNew, newPos, newRot, heightScale, widthScale, false,
        detectorInfoWsNew);

    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_1),
                     newPos + oldPosFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_2),
                     oldPosFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(FIRST_DET_INDEX_BANK_3),
                     newPos + oldPosFirstBank3);

    // Can these be made deterministic? This test is being added as a regression
    // test, so not a primary concern here to do so.
    const auto &expectedPositionBank1 = V3D(1.0, 2.032, 8.048);
    const auto &expectedPositionBank3 = V3D(1.0, 2.032, 18.048);

    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_1),
                     expectedPositionBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_2),
                     oldPosLastBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.position(LAST_DET_INDEX_BANK_3),
                     expectedPositionBank3);

    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_1),
                     newRot * oldRotFirstBank1);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_2),
                     oldRotFirstBank2);
    TS_ASSERT_EQUALS(detectorInfoWsNew.rotation(FIRST_DET_INDEX_BANK_3),
                     newRot * oldRotFirstBank3);
  }

private:
  detid_t FIRST_DET_ID_IN_BANK_1;
  size_t FIRST_DET_INDEX_BANK_1;
  detid_t FIRST_DET_ID_IN_BANK_2;
  size_t FIRST_DET_INDEX_BANK_2;
  detid_t FIRST_DET_ID_IN_BANK_3;
  size_t FIRST_DET_INDEX_BANK_3;

  detid_t LAST_DET_ID_IN_BANK_1;
  size_t LAST_DET_INDEX_BANK_1;
  detid_t LAST_DET_ID_IN_BANK_2;
  size_t LAST_DET_INDEX_BANK_2;
  detid_t LAST_DET_ID_IN_BANK_3;
  size_t LAST_DET_INDEX_BANK_3;

  MatrixWorkspace_sptr wsOld;
  MatrixWorkspace_sptr wsNew;

  V3D oldPosFirstBank1;
  V3D oldPosFirstBank2;
  V3D oldPosFirstBank3;

  V3D oldPosLastBank1;
  V3D oldPosLastBank2;
  V3D oldPosLastBank3;

  Quat oldRotFirstBank1;
  Quat oldRotFirstBank2;
  Quat oldRotFirstBank3;

  std::vector<std::string> bankNames = {"bank1", "bank3"};

  void setUpFixUpBankParameterMap() {
    wsOld = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
        3, 3, 3);
    wsNew = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
        3, 3, 3);

    const auto &detectorInfoWsNew = wsNew->detectorInfo();

    FIRST_DET_ID_IN_BANK_1 = 9;
    FIRST_DET_INDEX_BANK_1 = detectorInfoWsNew.indexOf(FIRST_DET_ID_IN_BANK_1);
    FIRST_DET_ID_IN_BANK_2 = 18;
    FIRST_DET_INDEX_BANK_2 = detectorInfoWsNew.indexOf(FIRST_DET_ID_IN_BANK_2);
    FIRST_DET_ID_IN_BANK_3 = 27;
    FIRST_DET_INDEX_BANK_3 = detectorInfoWsNew.indexOf(FIRST_DET_ID_IN_BANK_3);

    LAST_DET_ID_IN_BANK_1 = 17;
    LAST_DET_INDEX_BANK_1 = detectorInfoWsNew.indexOf(LAST_DET_ID_IN_BANK_1);
    LAST_DET_ID_IN_BANK_2 = 26;
    LAST_DET_INDEX_BANK_2 = detectorInfoWsNew.indexOf(LAST_DET_ID_IN_BANK_2);
    LAST_DET_ID_IN_BANK_3 = 35;
    LAST_DET_INDEX_BANK_3 = detectorInfoWsNew.indexOf(LAST_DET_ID_IN_BANK_3);

    const auto &detectorInfoOldWs = wsNew->detectorInfo();

    oldPosFirstBank1 = detectorInfoOldWs.position(FIRST_DET_INDEX_BANK_1);
    oldPosFirstBank2 = detectorInfoOldWs.position(FIRST_DET_INDEX_BANK_2);
    oldPosFirstBank3 = detectorInfoOldWs.position(FIRST_DET_INDEX_BANK_3);

    oldPosLastBank1 = detectorInfoOldWs.position(LAST_DET_INDEX_BANK_1);
    oldPosLastBank2 = detectorInfoOldWs.position(LAST_DET_INDEX_BANK_2);
    oldPosLastBank3 = detectorInfoOldWs.position(LAST_DET_INDEX_BANK_3);

    oldRotFirstBank1 = detectorInfoOldWs.rotation(FIRST_DET_INDEX_BANK_1);
    oldRotFirstBank2 = detectorInfoOldWs.rotation(FIRST_DET_INDEX_BANK_2);
    oldRotFirstBank3 = detectorInfoOldWs.rotation(FIRST_DET_INDEX_BANK_3);
  }
};

#endif /* MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_ */
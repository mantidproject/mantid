#ifndef MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENTTEST_H_
#define MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataHandling::RotateInstrumentComponent;

class RotateInstrumentComponentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RotateInstrumentComponentTest *createSuite() {
    return new RotateInstrumentComponentTest();
  }
  static void destroySuite(RotateInstrumentComponentTest *suite) {
    delete suite;
  }

  void test_Init() {
    RotateInstrumentComponent alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Exec_With_Absolute_Rotation() {
    using namespace Mantid::API;

    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            1, 1, false); // no monitors

    double angle = 45.0;
    Mantid::Kernel::V3D axis(0., 1., 1.);
    testWS = runRotateInstrument(testWS, "pixel-0)", angle, axis,
                                 false); // Detector 1

    const auto &spectrumInfo = testWS->spectrumInfo();
    const auto &detId1 = spectrumInfo.detector(0);
    auto expectedRot = Mantid::Kernel::Quat(angle, axis);
    auto newRot = detId1.getRotation();
    TS_ASSERT_DELTA(newRot.real(), expectedRot.real(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagI(), expectedRot.imagI(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagJ(), expectedRot.imagJ(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagK(), expectedRot.imagK(), 1e-12);
  }

  void test_Exec_With_Relative_Rotation() {
    using namespace Mantid::API;

    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            1, 1, false); // no monitors

    double angle = 45.0;
    Mantid::Kernel::V3D axis(0., 0., 1.);

    // Run it twice to check the relative rotation as the instrument starts out
    // with no rotation
    testWS = runRotateInstrument(testWS, "testInst", angle, axis,
                                 false); // instrument
    testWS = runRotateInstrument(testWS, "pixel-0)", angle, axis,
                                 true); // relative for detector

    auto expectedRot = Mantid::Kernel::Quat(angle, axis);
    expectedRot *= expectedRot;
    const auto &spectrumInfo = testWS->spectrumInfo();
    const auto &detId1 = spectrumInfo.detector(0);
    const auto &newRot = detId1.getRotation();

    TS_ASSERT_DELTA(newRot.real(), expectedRot.real(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagI(), expectedRot.imagI(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagJ(), expectedRot.imagJ(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagK(), expectedRot.imagK(), 1e-12);
  }

private:
  Mantid::API::MatrixWorkspace_sptr
  runRotateInstrument(const Mantid::API::MatrixWorkspace_sptr &testWS,
                      const std::string &compName, const double angle,
                      const Mantid::Kernel::V3D &axis, const bool relative) {
    RotateInstrumentComponent alg;
    alg.initialize();
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", compName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("X", axis.X()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Y", axis.Y()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Z", axis.Z()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Angle", angle));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelativeRotation", relative));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    return testWS;
  }
};

#endif /* MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENTTEST_H_ */

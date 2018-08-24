#ifndef MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/SpectrumInfo.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;

/** Test class for the helpers in MantidTestHelpers/WorkspaceCreationHelper.h */
class WorkspaceCreationHelperTest : public CxxTest::TestSuite {
public:
  void test_create2DWorkspaceWithRectangularInstrument() {
    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 10, 20);
    TS_ASSERT(ws);
    TS_ASSERT(ws->getInstrument());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2 * 100);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT(ws->getSpectrum(0).hasDetectorID(100));
    TS_ASSERT(ws->getSpectrum(1).hasDetectorID(101));
    TS_ASSERT_DELTA(ws->dataY(5)[0], 2.0, 1e-5);
  }

  void test_createEventWorkspaceWithFullInstrument() {
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10);
    TS_ASSERT(ws);
    TS_ASSERT(ws->getInstrument());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2 * 100);
    TS_ASSERT(ws->getSpectrum(0).hasDetectorID(100));
    TS_ASSERT(ws->getSpectrum(1).hasDetectorID(101));
  }

  void test_create2DWorkspaceWithValues() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace123(
        1, 2, false, std::set<int64_t>(), true);
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->size(), 2);
    TS_ASSERT(ws->hasDx(0));
    TS_ASSERT_EQUALS(ws->dx(0).rawData()[0], 2.);
    Workspace2D_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(
        1, 2, false, std::set<int64_t>(), false);
    TS_ASSERT(!ws2->hasDx(0));
  }

  void test_create2DWorkspaceWithReflectometryInstrumentMultiDetector() {
    constexpr double startX{0.};
    constexpr double detSize{0.023};
    auto ws = WorkspaceCreationHelper::
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(startX,
                                                                  detSize);
    const auto &spectrumInfo = ws->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.size(), 4)
    TS_ASSERT(spectrumInfo.isMonitor(0));
    TS_ASSERT_EQUALS(spectrumInfo.position(0), Mantid::Kernel::V3D(14, 0, 0))
    TS_ASSERT_EQUALS(spectrumInfo.position(1),
                     Mantid::Kernel::V3D(20, 5 - detSize, 0))
    TS_ASSERT_EQUALS(spectrumInfo.position(2), Mantid::Kernel::V3D(20, 5, 0))
    TS_ASSERT_EQUALS(spectrumInfo.position(3),
                     Mantid::Kernel::V3D(20, 5 + detSize, 0))
  }
};

#endif /* MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_ */

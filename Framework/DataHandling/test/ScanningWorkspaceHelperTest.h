#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ScanningWorkspaceHelper.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataHandling::ScanningWorkspaceHelper;

class ScanningWorkspaceHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScanningWorkspaceHelperTest *createSuite() {
    return new ScanningWorkspaceHelperTest();
  }
  static void destroySuite(ScanningWorkspaceHelperTest *suite) { delete suite; }

  void test_create_scanning_workspace_with_correct_time_ranges() {
    size_t nDetectors = 5;
    size_t nTimeIndexes = 4;
    size_t nBins = 10;

    const auto &wsWithInstrument =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            int(nDetectors), int(nBins));
    const auto &instrument = wsWithInstrument->getInstrument();

    std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 4}};

    auto builder = ScanningWorkspaceHelper(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(detectorInfo.scanInterval({i, j}), timeRanges[j]);
      }
    }
  }
};

#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_ */
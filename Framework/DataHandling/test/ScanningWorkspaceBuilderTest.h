#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ScanningWorkspaceBuilder.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataHandling::ScanningWorkspaceBuilder;

class ScanningWorkspaceBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScanningWorkspaceBuilderTest *createSuite() {
    return new ScanningWorkspaceBuilderTest();
  }
  static void destroySuite(ScanningWorkspaceBuilderTest *suite) {
    delete suite;
  }

  void test_create_scanning_workspace_with_too_small_instrument() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder =
        ScanningWorkspaceBuilder(nDetectors + 1, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(builder.setInstrument(instrument),
                            const std::logic_error &e, std::string(e.what()),
                            "There are not enough detectors in the instrument "
                            "for the number of detectors set in the scanning "
                            "workspace builder.");
  }

  void test_create_scanning_workspace_with_correct_time_ranges() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
  }

  void test_create_scanning_workspace_with_correct_time_durations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(0, timeDurations));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
  }

  void test_create_scanning_workspace_fails_if_no_instrument_set() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(builder.buildWorkspace(), const std::logic_error &e,
                            std::string(e.what()),
                            "Can not build workspace - instrument has not been "
                            "set. Please call setInstrument() before "
                            "building.");
  }

  void test_create_scanning_workspace_fails_if_no_time_ranges_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));

    TS_ASSERT_THROWS_EQUALS(builder.buildWorkspace(), const std::logic_error &e,
                            std::string(e.what()),
                            "Can not build workspace - time ranges have not "
                            "been set. Please call setTimeRanges() before "
                            "building.");
  }

  void
  test_create_scanning_workspace_fails_if_time_ranges_have_the_wrong_dimensions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    std::vector<std::pair<DateAndTime, DateAndTime>> timeRangesWrongSize = {
        {0, 1}, {1, 2}};

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_EQUALS(builder.setTimeRanges(timeRangesWrongSize),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of start time, end time pairs supplied "
                            "does not match the number of time indexes.");
  }

  void
  test_create_scanning_workspace_fails_if_time_durations_have_the_wrong_dimensions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    std::vector<double> timeDurationsWrongSize = {0, 1e-9};

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_EQUALS(builder.setTimeRanges(0, timeDurationsWrongSize),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of time durations supplied does not match "
                            "the number of time indexes.");
  }

  void test_creating_workspace_with_positions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(positions));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(V3D(double(i), double(j), 1.0),
                         detectorInfo.position({i, j}))
      }
    }
  }

  void test_creating_workspace_with_positions_with_too_many_detectors() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initalisePositions(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setPositions(positions), const std::logic_error &e,
        std::string(e.what()),
        "Number of positions supplied does not match the number of detectors.");
  }

  void test_creating_workspace_with_positions_with_too_many_time_indexes() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setPositions(positions),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of positions supplied does not match the "
                            "number of time indexes.");
  }

  void test_creating_workspace_with_rotations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRotations(rotations));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        auto quat = Quat(double(i), double(j), 1.0, 2.0);
        quat.normalize();
        TS_ASSERT_EQUALS(quat, detectorInfo.rotation({i, j}))
      }
    }
  }

  void test_creating_workspace_with_rotations_with_too_many_detectors() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initaliseRotations(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setRotations(rotations), const std::logic_error &e,
        std::string(e.what()),
        "Number of rotations supplied does not match the number of detectors.");
  }

  void test_creating_workspace_with_rotations_with_too_many_time_indexes() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initaliseRotations(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setRotations(rotations),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of rotations supplied does not match the "
                            "number of time indexes.");
  }

private:
  size_t nDetectors = 5;
  size_t nTimeIndexes = 4;
  size_t nBins = 10;

  std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges = {
      {0, 1}, {1, 3}, {3, 6}, {6, 10}};

  std::vector<double> timeDurations = {1e-9, 2e-9, 3e-9, 4e-9};

  std::vector<std::vector<V3D>> positions;
  std::vector<std::vector<Quat>> rotations;

  void initalisePositions(size_t nDetectors, size_t nTimeIndexes) {
    for (size_t i = 0; i < nDetectors; ++i) {
      std::vector<V3D> timePositions;
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        timePositions.push_back(V3D(double(i), double(j), 1.0));
      }
      positions.push_back(timePositions);
    }
  }

  void initaliseRotations(size_t nDetectors, size_t nTimeIndexes) {
    for (size_t i = 0; i < nDetectors; ++i) {
      std::vector<Quat> timeRotations;
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        timeRotations.push_back(Quat(double(i), double(j), 1.0, 2.0));
      }
      rotations.push_back(timeRotations);
    }
  }

  Instrument_const_sptr createSimpleInstrument(size_t nDetectors,
                                               size_t nBins) {
    const auto &wsWithInstrument =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            int(nDetectors), int(nBins));
    return wsWithInstrument->getInstrument();
  }

  void checkTimeRanges(const DetectorInfo &detectorInfo) {
    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(detectorInfo.scanInterval({i, j}), timeRanges[j]);
      }
    }
  }
};

#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_ */
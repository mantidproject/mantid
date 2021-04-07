// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

#include "MantidAPI/Run.h"
#include "MantidDataObjects/FakeMD.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

class FakeMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FakeMDTest *createSuite() { return new FakeMDTest(); }
  static void destroySuite(FakeMDTest *suite) { delete suite; }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------
  void test_empty_peak_and_uniform_params_throws_exception() {
    using Mantid::DataObjects::FakeMD;

    const std::vector<double> peakParams;
    const std::vector<double> uniformParams;
    const std::vector<double> ellipsoidParams;
    const int randomSeed(0);
    const bool randomizeSignal(false);

    TS_ASSERT_THROWS(FakeMD(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal),
                     const std::invalid_argument &);
  }

  //---------------------------------------------------------------------------
  // Success cases
  //---------------------------------------------------------------------------

  void test_no_randomize() {
    using Mantid::DataObjects::FakeMD;
    using Mantid::DataObjects::MDEventsTestHelper::makeMDEW;

    // Destination workspace
    auto fakeData = makeMDEW<3>(10, 0.0, 10.0, 1);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 1000);

    const std::vector<double> peakParams = {1000.0, 5.0, 5.0, 5.0, 1.0};
    const std::vector<double> uniformParams = {10000.0};
    const std::vector<double> ellipsoidParams = {500.0, 5.0, 5.0, 5.0, 1.0, 0.0, 0.0, 0.0, 1.0,
                                                 0.0,   0.0, 0.0, 1.0, 0.5, 0.5, 0.5, -1.0};
    const int randomSeed(0);
    const bool randomizeSignal(false);

    FakeMD faker(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal);
    faker.fill(fakeData);
    // Now there are 11000 more points.
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 12500);
  }

  void test_ellipsoid_counts() {
    using Mantid::DataObjects::FakeMD;
    using Mantid::DataObjects::MDEventsTestHelper::makeMDEW;

    // Destination workspace
    auto fakeData = makeMDEW<3>(10, 0.0, 10.0, 0);

    const std::vector<double> peakParams;
    const std::vector<double> uniformParams;
    const std::vector<double> ellipsoidParams = {2000.0, 5.0, 5.0, 5.0, 1.0, 0.0, 0.0, 0.0, 1.0,
                                                 0.0,    0.0, 0.0, 1.0, 0.5, 0.5, 0.5, 1.0};
    const int randomSeed(0);
    const bool randomizeSignal(false);

    FakeMD faker(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal);
    faker.fill(fakeData);

    auto Npts = fakeData->getNPoints();
    TS_ASSERT_EQUALS(Npts, ellipsoidParams[0]);
    // avg of counts converges to 0.2175 for 3D multivariate gaussian
    TS_ASSERT_DELTA(fakeData->getBox()->getSignal(), static_cast<double>(Npts) * 0.2175,
                    static_cast<double>(Npts) * 0.0015);
  }

  void test_exec_randomizeSignal() {
    using Mantid::DataObjects::FakeMD;
    using Mantid::DataObjects::MDEventsTestHelper::makeMDEW;

    auto fakeData = makeMDEW<3>(10, 0.0, 10.0, 0);
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 0);
    TS_ASSERT_DELTA(fakeData->getBox()->getSignal(), 0.0, 1e-5);

    const std::vector<double> peakParams = {100.0, 5.0, 5.0, 5.0, 1.0};
    const std::vector<double> uniformParams = {100.0};
    const std::vector<double> ellipsoidParams;
    const int randomSeed(0);
    const bool randomizeSignal(true);

    FakeMD faker(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal);
    faker.fill(fakeData);

    // Now there are 200 more points.
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 200);
    // 200 +- 100 signal
    TS_ASSERT_DELTA(fakeData->getBox()->getSignal(), 200.0, 100);
    TS_ASSERT_DELTA(fakeData->getBox()->getErrorSquared(), 200.0, 100);
    // But not exactly 200
    TS_ASSERT_DIFFERS(fakeData->getBox()->getSignal(), 200.0);
    TS_ASSERT_DIFFERS(fakeData->getBox()->getErrorSquared(), 200.0);

    TSM_ASSERT("If the workspace is file-backed, then it needs updating.", fakeData->fileNeedsUpdating());
  }

  void testExecRegularSignal() {
    using Mantid::DataObjects::FakeMD;
    using Mantid::DataObjects::MDEventsTestHelper::makeMDEW;

    auto fakeData = makeMDEW<3>(10, 0.0, 10.0, 0);

    // No events
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 0);
    TS_ASSERT_DELTA(fakeData->getBox()->getSignal(), 0.0, 1e-5);

    const std::vector<double> peakParams;
    const std::vector<double> uniformParams = {-1000.0};
    const std::vector<double> ellipsoidParams;
    const int randomSeed(0);
    const bool randomizeSignal(false);

    FakeMD faker(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal);
    faker.fill(fakeData);

    // Now there are 1000 more points.
    TS_ASSERT_EQUALS(fakeData->getNPoints(), 1000);
    TS_ASSERT_DELTA(fakeData->getBox()->getSignal(), 1000.0, 1.e-6);
    TS_ASSERT_DELTA(fakeData->getBox()->getErrorSquared(), 1000.0, 1.e-6);

    TSM_ASSERT("If the workspace is file-backed, then it needs updating.", fakeData->fileNeedsUpdating());
  }

  void test_Creating_Full_MDEvents_Adds_DetectorIDs_To_Workspace() {
    using Mantid::DataObjects::FakeMD;
    using Mantid::DataObjects::MDEvent;
    using Mantid::DataObjects::MDEventsTestHelper::makeAnyMDEW;
    using Mantid::Kernel::PropertyWithValue;

    auto fakeData = makeAnyMDEW<MDEvent<3>, 3>(10, 0.0, 10.0, 0);
    // Give it an instrument
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 16);
    auto ei = fakeData->getExperimentInfo(0);
    ei->setInstrument(inst);
    // Give it a run number
    ei->mutableRun().addProperty(new PropertyWithValue<std::string>("run_number", "12345"), true);

    const std::vector<double> peakParams;
    const std::vector<double> uniformParams = {-1000.0};
    const std::vector<double> ellipsoidParams;
    const int randomSeed(0);
    const bool randomizeSignal(false);

    FakeMD faker(uniformParams, peakParams, ellipsoidParams, randomSeed, randomizeSignal);
    faker.fill(fakeData);

    TS_ASSERT_EQUALS(1000, fakeData->getNEvents());

    Mantid::detid_t expectedIDs[10] = {37, 235, 140, 72, 255, 137, 203, 133, 79, 192};
    auto it = fakeData->createIterator();
    size_t counter(0);
    while (counter < 10) {
      int32_t id = it->getInnerDetectorID(0);
      TS_ASSERT_EQUALS(expectedIDs[counter], id);
      it->next();
      ++counter;
    }
  }
};

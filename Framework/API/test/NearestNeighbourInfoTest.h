#ifndef MANTID_API_NEARESTNEIGHBOURINFOTEST_H_
#define MANTID_API_NEARESTNEIGHBOURINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidAPI/NearestNeighbourInfo.h"
#include "MantidAPI/SpectrumInfo.h"

using Mantid::API::NearestNeighbourInfo;

class NearestNeighbourInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NearestNeighbourInfoTest *createSuite() {
    return new NearestNeighbourInfoTest();
  }
  static void destroySuite(NearestNeighbourInfoTest *suite) { delete suite; }

  NearestNeighbourInfoTest() {
    workspace.initialize(100, 1, 1);
    InstrumentCreationHelper::addFullInstrumentToWorkspace(workspace, false,
                                                           false, "");
    workspace.rebuildSpectraMapping();
    workspace.getSpectrum(0).clearData();
    workspace.mutableSpectrumInfo().setMasked(0, true);
  }

  void test_construct() {
    TS_ASSERT_THROWS_NOTHING(NearestNeighbourInfo(workspace, false));
  }

  void test_neighbourCount() {
    // No detailed test, just checking if parameters are passed on to
    // NearestNeighbours correctly.
    NearestNeighbourInfo nn2(workspace, false, 2);
    TS_ASSERT_EQUALS(nn2.getNeighboursExact(3).size(), 2);
    NearestNeighbourInfo nn4(workspace, false, 4);
    const auto neighbours = nn4.getNeighboursExact(3);
    TS_ASSERT_EQUALS(neighbours.size(), 4);
    TS_ASSERT_EQUALS(neighbours.count(1), 1);
  }

  void test_neighbourCount_ignoreMasked() {
    // No detailed test, just checking if parameters are passed on to
    // NearestNeighbours correctly.
    NearestNeighbourInfo nn2(workspace, true, 2);
    TS_ASSERT_EQUALS(nn2.getNeighboursExact(3).size(), 2);
    NearestNeighbourInfo nn4(workspace, true, 4);
    const auto neighbours = nn4.getNeighboursExact(3);
    TS_ASSERT_EQUALS(neighbours.size(), 4);
    TS_ASSERT_EQUALS(neighbours.count(1), 0);
  }

private:
  WorkspaceTester workspace;
};

#endif /* MANTID_API_NEARESTNEIGHBOURINFOTEST_H_ */

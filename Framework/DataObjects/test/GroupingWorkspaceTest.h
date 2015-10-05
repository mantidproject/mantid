#ifndef MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_

#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class GroupingWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    GroupingWorkspace_sptr ws(new GroupingWorkspace());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",
                               ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",
                               ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING(ws->initialize(100, 1, 1));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
  }

  void test_constructor_from_Instrument() {
    // Fake instrument with 5*9 pixels with ID starting at 1
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(),
                     "basic"); // Name of the test instrument
    std::set<detid_t> dets = ws->getSpectrum(0)->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    // Set the group numbers
    for (int group = 0; group < 5; group++)
      for (int i = 0; i < 9; i++)
        ws->dataY(group * 9 + i)[0] = double(group + 1);

    // Get the map
    std::map<detid_t, int> map;
    int64_t ngroups;
    ws->makeDetectorIDToGroupMap(map, ngroups);

    TS_ASSERT_EQUALS(ngroups, 5);

    TS_ASSERT_EQUALS(map[1], 1);
    TS_ASSERT_EQUALS(map[9], 1);
    TS_ASSERT_EQUALS(map[10], 2);
    TS_ASSERT_EQUALS(map[45], 5);
  }

  void testClone() {
    // As test_constructor_from_Instrument(), set on ws, get on clone.
    // Fake instrument with 5*9 pixels with ID starting at 1
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));
    auto cloned = ws->clone();

    TS_ASSERT_EQUALS(cloned->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS(cloned->blocksize(), 1);
    TS_ASSERT_EQUALS(cloned->getInstrument()->getName(),
                     "basic"); // Name of the test instrument
    std::set<detid_t> dets = cloned->getSpectrum(0)->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    // Set the group numbers
    for (int group = 0; group < 5; group++)
      for (int i = 0; i < 9; i++)
        ws->dataY(group * 9 + i)[0] = double(group + 1);
    cloned = ws->clone();

    // Get the map
    std::map<detid_t, int> map;
    int64_t ngroups;
    cloned->makeDetectorIDToGroupMap(map, ngroups);

    TS_ASSERT_EQUALS(ngroups, 5);

    TS_ASSERT_EQUALS(map[1], 1);
    TS_ASSERT_EQUALS(map[9], 1);
    TS_ASSERT_EQUALS(map[10], 2);
    TS_ASSERT_EQUALS(map[45], 5);
  }
};

#endif /* MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_ */

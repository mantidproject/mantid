// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Timer.h"
#include "PropertyManagerHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace { // anonymous namespace
// copied from body of ComponentCreationHelper::createTestInstrumentCylindrical
const std::size_t PIXELS_PER_BANK = 9;
} // namespace

class GroupingWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    GroupingWorkspace_sptr ws(new GroupingWorkspace());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.", ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.", ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING(ws->initialize(100, 1, 1));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
  }

  void test_constructor_from_Instrument() {
    // Fake instrument with 5*9 pixels with ID starting at 1
    const std::size_t NUM_BANKS = 5;
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANKS);

    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), NUM_BANKS * PIXELS_PER_BANK);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(),
                     "basic"); // Name of the test instrument
    auto dets = ws->getSpectrum(0).getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    // Set the group numbers
    for (std::size_t group = 0; group < NUM_BANKS; group++)
      for (std::size_t i = 0; i < PIXELS_PER_BANK; i++)
        ws->dataY(group * PIXELS_PER_BANK + i)[0] = double(group + 1);

    // Get the map
    std::map<detid_t, int> map;
    int64_t ngroups;
    ws->makeDetectorIDToGroupMap(map, ngroups);

    TS_ASSERT_EQUALS(ngroups, NUM_BANKS);

    TS_ASSERT_EQUALS(map[1], 1);
    TS_ASSERT_EQUALS(map[9], 1);
    TS_ASSERT_EQUALS(map[10], 2);
    TS_ASSERT_EQUALS(map[45], 5);
  }

  void testClone() {
    // As test_constructor_from_Instrument(), set on ws, get on clone.
    // Fake instrument with 5*9 pixels with ID starting at 1
    const std::size_t NUM_BANKS = 5;
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANKS);

    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));
    auto cloned = ws->clone();

    TS_ASSERT_EQUALS(cloned->getNumberHistograms(), NUM_BANKS * PIXELS_PER_BANK);
    TS_ASSERT_EQUALS(cloned->blocksize(), 1);
    TS_ASSERT_EQUALS(cloned->getInstrument()->getName(),
                     "basic"); // Name of the test instrument
    auto dets = cloned->getSpectrum(0).getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    // Set the group numbers
    for (std::size_t group = 0; group < NUM_BANKS; group++)
      for (std::size_t i = 0; i < PIXELS_PER_BANK; i++)
        ws->dataY(group * PIXELS_PER_BANK + i)[0] = double(group + 1);
    cloned = ws->clone();

    // Get the map
    std::map<detid_t, int> map;
    int64_t ngroups;
    cloned->makeDetectorIDToGroupMap(map, ngroups);

    TS_ASSERT_EQUALS(ngroups, NUM_BANKS);

    TS_ASSERT_EQUALS(map[1], 1);
    TS_ASSERT_EQUALS(map[9], 1);
    TS_ASSERT_EQUALS(map[10], 2);
    TS_ASSERT_EQUALS(map[45], 5);
  }

  /**
   * Test declaring an input workspace property and retrieving as const_sptr or
   * sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    GroupingWorkspace_sptr wsInput(new GroupingWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    GroupingWorkspace_const_sptr wsConst;
    GroupingWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<GroupingWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<GroupingWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    GroupingWorkspace_const_sptr wsCastConst;
    GroupingWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (GroupingWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (GroupingWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  void testGetTotalGroups() {
    GroupingWorkspace_sptr ws(new GroupingWorkspace());
    // create a groupingworkspace with 2 groups
    ws->initialize(100, 1, 1);
    ws->dataY(0)[0] = 1;
    ws->dataY(1)[0] = 2;
    TS_ASSERT_EQUALS(ws->getTotalGroups(), 3);
  }

  void testGetGroupIDs() {
    GroupingWorkspace_sptr ws(new GroupingWorkspace());
    // create a groupingworkspace with 2 groups
    ws->initialize(100, 1, 1);
    ws->dataY(0)[0] = 1;
    ws->dataY(1)[0] = 2;
    TS_ASSERT_EQUALS(ws->getGroupIDs().size(), 3);
    TS_ASSERT_EQUALS(ws->getGroupIDs()[0], -1);
    TS_ASSERT_EQUALS(ws->getGroupIDs()[1], 1);
    TS_ASSERT_EQUALS(ws->getGroupIDs()[2], 2);
  }

  void testDetIDsOfGroup() {
    const std::size_t NUM_BANKS = 3;
    // As test_constructor_from_Instrument(), set on ws, get on clone.
    // Fake instrument with 3*9 pixels with ID starting at 1
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANKS);
    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));
    // verify that the correct thing was made
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), NUM_BANKS * PIXELS_PER_BANK);

    // create a groupingworkspace with 1/3 in group 1, 1/3 in group 2, and 1/3 unassigned
    for (detid_t detid = 1; detid < detid_t(NUM_BANKS * PIXELS_PER_BANK + 1); ++detid) {
      if (detid % 3 == 0)
        ws->setValue(detid, 1);
      else if ((detid + 1) % 3 == 0)
        ws->setValue(detid, 2);
      // leave the others going to group -1
    }

    // verify that the group ids to check exist
    TS_ASSERT_EQUALS(ws->getGroupIDs(), std::vector({-1, 1, 2}));

    // 0 is not a valid id, -1 is not set
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(-1).size(), PIXELS_PER_BANK);
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(-1), std::vector({1, 4, 7, 10, 13, 16, 19, 22, 25}));
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(1).size(), PIXELS_PER_BANK);
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(1), std::vector({3, 6, 9, 12, 15, 18, 21, 24, 27}));
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(2).size(), PIXELS_PER_BANK);
    TS_ASSERT_EQUALS(ws->getDetectorIDsOfGroup(2), std::vector({2, 5, 8, 11, 14, 17, 20, 23, 26}));
  }
};

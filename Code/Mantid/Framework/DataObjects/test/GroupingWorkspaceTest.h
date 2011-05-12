#ifndef MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_

#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;

class GroupingWorkspaceTest : public CxxTest::TestSuite
{
public:

  void test_default_constructor()
  {
    GroupingWorkspace_sptr ws(new GroupingWorkspace());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING( ws->initialize(100, 1, 1) );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
  }

  void test_constructor_from_Instrument()
  {
    // Fake instrument with 5*9 pixels
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    GroupingWorkspace_sptr ws(new GroupingWorkspace(inst));

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
    TS_ASSERT_EQUALS( ws->getInstrument()->getName(), "basic"); // Name of the test instrument
    TS_ASSERT_EQUALS( ws->spectraMap().nElements(), 45);
    std::vector<int64_t> dets = ws->spectraMap().getDetectors(0);
    TS_ASSERT_EQUALS(dets.size(), 1);

    // Set the group numbers
    for (int group=0; group<5; group++)
      for (int i=0; i<9; i++)
        ws->dataY(group*9+i)[0] = double(group+1);

    // Get the map
    std::map<int64_t,int64_t> map;
    int64_t ngroups;
    ws->makeDetectorIDToGroupMap(map, ngroups);

    TS_ASSERT_EQUALS(ngroups, 5);

    TS_ASSERT_EQUALS( map[1], 1 );
    TS_ASSERT_EQUALS( map[9], 1 );
    TS_ASSERT_EQUALS( map[10], 2 );
    TS_ASSERT_EQUALS( map[45], 5 );
  }


};


#endif /* MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_ */


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
    std::vector<int> dets = ws->spectraMap().getDetectors(0);
    TS_ASSERT_EQUALS(dets.size(), 1);
  }


};


#endif /* MANTID_DATAOBJECTS_GROUPINGWORKSPACETEST_H_ */


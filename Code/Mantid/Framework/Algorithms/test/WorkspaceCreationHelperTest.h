#ifndef MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidAPI/SpectraDetectorTypes.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/** Test class for the helpers in MantidTestHelpers/WorkspaceCreationHelper.h */
class WorkspaceCreationHelperTest : public CxxTest::TestSuite
{
public:

  void test_create2DWorkspaceWithRectangularInstrument()
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20);
    TS_ASSERT(ws);
    TS_ASSERT( ws->getInstrument() );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 2*100);
    TS_ASSERT_EQUALS( ws->blocksize(), 20);
    index2detid_map * map = ws->getWorkspaceIndexToDetectorIDMap();
    TS_ASSERT_EQUALS( map->at(0), 100);
    TS_ASSERT_EQUALS( map->at(1), 101);
    TS_ASSERT_DELTA( ws->dataY(5)[0], 2.0, 1e-5);
  }

  void test_createEventWorkspaceWithFullInstrument()
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10);
    TS_ASSERT(ws);
    TS_ASSERT( ws->getInstrument() );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 2*100);
    index2detid_map * map = ws->getWorkspaceIndexToDetectorIDMap();
    TS_ASSERT_EQUALS( map->at(0), 100);
    TS_ASSERT_EQUALS( map->at(1), 101);
  }


};


#endif /* MANTID_ALGORITHMS_WORKSPACECREATIONHELPERTEST_H_ */


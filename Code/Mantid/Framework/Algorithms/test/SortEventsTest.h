#ifndef MANTID_ALGORITHMS_SORTEVENTSTEST_H_
#define MANTID_ALGORITHMS_SORTEVENTSTEST_H_

#include "MantidAlgorithms/SortEvents.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class SortEventsTest : public CxxTest::TestSuite
{
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  SortEventsTest()
  {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }


  void testSortByTof()
  {
    std::string wsName("test_inEvent3");
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    Workspace2D_sptr test_in_ws2d = WorkspaceCreationHelper::Create2DWorkspaceBinned(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add("workspace2d", test_in_ws2d);

    SortEvents sort;
    sort.initialize();
    //Not an event workspace
    TS_ASSERT_THROWS( sort.setPropertyValue("InputWorkspace","workspace2d"), std::invalid_argument);
    //This one will be ok
    sort.setPropertyValue("InputWorkspace",wsName);
    sort.setPropertyValue("SortBy", "X Value");

    TS_ASSERT(sort.execute());
    TS_ASSERT(sort.isExecuted());

    EventWorkspace_const_sptr outWS = boost::dynamic_pointer_cast<const EventWorkspace>(AnalysisDataService::Instance().retrieve(wsName));

    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), NUMBINS);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].tof(), ve[i+1].tof());

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove("workspace2d");

  }


  void testSortByPulseTime()
  {
    std::string wsName("test_inEvent4");
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    SortEvents sort;
    sort.initialize();
    sort.setPropertyValue("InputWorkspace",wsName);
    sort.setPropertyValue("SortBy", "Pulse Time");
    TS_ASSERT(sort.execute());
    TS_ASSERT(sort.isExecuted());

    EventWorkspace_const_sptr outWS = boost::dynamic_pointer_cast<const EventWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), NUMBINS);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].pulseTime(), ve[i+1].pulseTime());

    AnalysisDataService::Instance().remove(wsName);

  }


};


#endif /* MANTID_ALGORITHMS_SORTEVENTSTEST_H_ */


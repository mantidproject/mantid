#ifndef SORTTEST_H_
#define SORTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Sort.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class SortTest : public CxxTest::TestSuite
{
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  SortTest()
  {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }


  void testSortByTof()
  {
    std::string wsName("test_inEvent3");
    EventWorkspace_sptr test_in = CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    Workspace2D_sptr test_in_ws2d = Create2DWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add("workspace2d", test_in_ws2d);

    Sort sort;
    sort.initialize();
    //Not an event workspace
    TS_ASSERT_THROWS( sort.setPropertyValue("InputWorkspace","workspace2d"), std::invalid_argument);
    //This one will be ok
    sort.setPropertyValue("InputWorkspace",wsName);
    sort.setPropertyValue("SortBy", "Time of Flight");

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
    EventWorkspace_sptr test_in = CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    Sort sort;
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


private:

  EventWorkspace_sptr CreateRandomEventWorkspace(int numbins, int numpixels)
  {
    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numpixels,numbins,numbins-1);

    //Create the original X axis to histogram on.
    //Create the x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(numbins);
    for (int i = 0; i < numbins; ++i)
      xRef[i] = i*BIN_DELTA;


    //Make up some data for each pixels
    for (int i=0; i< numpixels; i++)
    {
      //Create one event for each bin
      EventList& events = retVal->getEventListAtPixelID(i);
      for (double ie=0; ie<numbins; ie++)
      {
        //Create a list of events, randomize
        events += TofEvent( std::rand() , std::rand());
      }
   }
    retVal->doneLoadingData();
    retVal->setAllX(axis);


    return retVal;
  }


  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen-1,sqrt(3.0)));
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;
    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
      j+=1.5;
    }
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }
    return retVal;
  }


};

#endif /* SORTTEST_H_ */

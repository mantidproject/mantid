/*
 * EventWorkspaceTest.h
 *
 *  Created on: May 28, 2010
 *      Author: Janik Zikovsky
 */

#ifndef EVENTWORKSPACETEST_H_
#define EVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "MantidKernel/Timer.h"

#ifndef _WIN32
  #include <sys/resource.h>
#endif


using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;
using namespace boost::posix_time;


//==========================================================================================
class EventWorkspaceTest : public CxxTest::TestSuite
{
private:
  EventWorkspace_sptr ew;
  int NUMPIXELS, NUMBINS, NUMEVENTS, BIN_DELTA;

public:

  EventWorkspaceTest()
  {
    NUMPIXELS = 500;
    NUMBINS = 1025;
    NUMEVENTS = 100;
    BIN_DELTA = 1000;
  }


  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  EventWorkspace_sptr createEventWorkspace(int initialize_pixels, int setX)
  {

    EventWorkspace_sptr retVal(new EventWorkspace);
    if (initialize_pixels)
      retVal->initialize(NUMPIXELS,1,1);
    else
      retVal->initialize(1,1,1);

    //Make fake events
    for (int pix=0; pix<NUMPIXELS; pix++)
    {
      for (int i=0; i<NUMEVENTS; i++)
      {
        //Two events per bin
        retVal->getEventListAtPixelID(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
        retVal->getEventListAtPixelID(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
      }
    }
    retVal->doneLoadingData();

    if (setX)
    {
      //Create the x-axis for histogramming.
      Kernel::cow_ptr<MantidVec> axis;
      MantidVec& xRef = axis.access();
      xRef.resize(NUMBINS);
      for (int i = 0; i < NUMBINS; ++i)
        xRef[i] = i*BIN_DELTA;

      //Try setting a single axis, make sure it doesn't throw
      retVal->setX(2, axis);

      //Set all the histograms at once.
      retVal->setAllX(axis);
    }

    return retVal;
  }


  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   * 2 events per bin
   */
  EventWorkspace_sptr createFlatEventWorkspace()
  {
    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(NUMPIXELS,1,1);

    //Make fake events
    for (int pix=0; pix<NUMPIXELS; pix++)
    {
      for (int i=0; i<NUMBINS-1; i++)
      {
        //Two events per bin
        retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*BIN_DELTA, 1);
        retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*BIN_DELTA, 1);
      }
    }
    retVal->doneLoadingData();

    //Create the x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(NUMBINS);
    for (int i = 0; i < NUMBINS; ++i)
      xRef[i] = i*BIN_DELTA;
    //Set all the histograms at once.
    retVal->setAllX(axis);
    return retVal;
  }



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


  //------------------------------------------------------------------------------
  void setUp()
  {
    ew = createEventWorkspace(1, 1);
  }


  //------------------------------------------------------------------------------
  void test_constructor()
  {
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), NUMBINS-1);
    TS_ASSERT_EQUALS( ew->size(), (NUMBINS-1)*NUMPIXELS);

    //Are the returned arrays the right size?
    const EventList el(ew->getEventList(1));
    TS_ASSERT_EQUALS( el.dataX().size(), NUMBINS);
    TS_ASSERT_EQUALS( el.dataY()->size(), NUMBINS-1);
    TS_ASSERT_EQUALS( el.dataE()->size(), NUMBINS-1);
    TS_ASSERT( el.hasDetectorID(1) );

    //Don't access data after doneLoadingData
    TS_ASSERT_THROWS( ew->getEventListAtPixelID(12), std::runtime_error);
  }

  //------------------------------------------------------------------------------
  void testgetOrAddEventList()
  {
    //Pick some workspace index
    EventList& el = ew->getOrAddEventList(1023);
    //Now you got lots more histograms
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), 1023+1);
    TS_ASSERT_EQUALS( el.getNumberEvents(), 0);
    TS_ASSERT_EQUALS( el.getDetectorIDs().size(), 0);
    TS_ASSERT( !el.hasDetectorID(1023) );

    ew->doneAddingEventLists();
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), 1023+1);
    //but there are still only 500 entries in the spectra map, since only 500 of them have detectors
    TS_ASSERT_EQUALS( ew->spectraMap().nElements(), NUMPIXELS);

  }

  //------------------------------------------------------------------------------
  void test_getMemorySize()
  {
    // Because of the way vectors allocate, we can only know the minimum amount of memory that can be used.
    size_t min_memory = (ew->getNumberEvents() * sizeof(TofEvent) + NUMPIXELS * sizeof(EventList))/1024;
    TS_ASSERT_LESS_THAN_EQUALS(min_memory,  ew->getMemorySize());
  }

  //------------------------------------------------------------------------------
  void test_destructor()
  {
    EventWorkspace * ew2 = new EventWorkspace();
    delete ew2;
  }


  //------------------------------------------------------------------------------
  void test_constructor_not_settingx()
  {
    //Do the workspace, but don't set x
    ew = createEventWorkspace(1, 0);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), 0);
    TS_ASSERT_EQUALS( ew->size(), 0);

    //Didn't set X? well all the histograms are size 0
    const EventList el(ew->getEventList(1));
    TS_ASSERT_EQUALS( el.dataX().size(), 0);
    TS_ASSERT_EQUALS( el.dataY()->size(), 0);
    TS_ASSERT_EQUALS( el.dataE()->size(), 0);

  }



  //------------------------------------------------------------------------------
  void test_padPixels()
  {
    bool timing = false;
    ew = createEventWorkspace(1, 0);

    int numpixels = timing ? 900000 : 1800;
    //Make an instrument with lots of pixels
    ew->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(numpixels/9));

    Timer timer;
    ew->padPixels(false);
    if (timing) std::cout << "\n" << timer.elapsed() << " seconds for padPixels(false).\n";

    TS_ASSERT_EQUALS(ew->getNumberHistograms(), numpixels);
    int badcount = 0;
    for (int i=0; i < numpixels; i++)
    {
      bool b = ew->getEventList(i).hasDetectorID(i+1);
      TSM_ASSERT("ew->getEventList(i).hasDetectorID(i+1)", b);
      if (b)
        if (badcount++ > 40) break;
    }

    IndexToIndexMap *map = ew->getWorkspaceIndexToDetectorIDMap();
    TS_ASSERT_EQUALS(map->size(), numpixels);
    TS_ASSERT_EQUALS((*map)[50], 51); //for example
    delete map;

    if (timing)
    {
      ew->clearData();
      Timer timer2;
      ew->padPixels(true);
      std::cout << "\n" << timer2.elapsed() << " seconds for padPixels(true).\n";

      padPixels_manually_timing(numpixels);
    }

  }

  //------------------------------------------------------------------------------
  void padPixels_manually_timing(int numpixels)
  {
    EventWorkspace_sptr ew(new EventWorkspace);
    ew->initialize(NUMPIXELS,1,1);
    //Make an instrument with lots of pixels
    ew->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(numpixels/9));

    Timer timer;

    std::map<int, Geometry::IDetector_sptr> detector_map = ew->getInstrument()->getDetectors();
    std::map<int, Geometry::IDetector_sptr>::iterator it;
    for (it = detector_map.begin(); it != detector_map.end(); it++)
    {
      //Go through each pixel in the map, but forget monitors.
      if (!it->second->isMonitor())
      {
        // and simply get the event list. It will be created if it was not there already.
        ew->getEventListAtPixelID(it->first); //it->first is detector ID #
      }
    }
    ew->doneLoadingData();

    std::cout << "\n" << timer.elapsed() << " seconds for padPixels done manually.\n";
    TS_ASSERT_EQUALS(ew->getNumberHistograms(), numpixels);
  }




  //------------------------------------------------------------------------------
  void test_uneven_pixel_ids()
  {
    EventWorkspace_sptr uneven(new EventWorkspace);
    uneven->initialize(1,1,1);

    //Make fake events. Pixel IDs start at 5 increment by 10
    for (int pix=5; pix<NUMPIXELS; pix += 10)
    {
      for (int i=0; i<pix; i++)
      {
        uneven->getEventListAtPixelID(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
      }
    }
    uneven->doneLoadingData();

    //Create the x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(NUMBINS);
    for (int i = 0; i < NUMBINS; ++i)
      xRef[i] = i*BIN_DELTA;
    //Set all the histograms at once.
    uneven->setAllX(axis);

    TS_ASSERT_EQUALS( uneven->getNumberHistograms(), NUMPIXELS/10);
    TS_ASSERT_EQUALS( uneven->blocksize(), (NUMBINS-1));
    TS_ASSERT_EQUALS( uneven->size(), (NUMBINS-1)*NUMPIXELS/10);

    //Axis 1 is the map between spectrum # and the workspace index.
    //  It should be a dumb 1-1 map
    TS_ASSERT_EQUALS( uneven->getAxis(1)->spectraNo(0), 0 );
    TS_ASSERT_EQUALS( uneven->getAxis(1)->spectraNo(5), 5 );
    TS_ASSERT_EQUALS( uneven->getAxis(1)->length(), NUMPIXELS/10 );

    //The spectra map should take each workspace index and point to the right pixel id: 5,15,25, etc.
    for (int i=0; i<uneven->getNumberHistograms(); i++)
      TS_ASSERT_EQUALS( uneven->spectraMap().getDetectors(i)[0], 5 + i*10);

    //Workspace index 0 is at pixelid 5 and has 5 events
    const EventList el0(uneven->getEventList(0));
    TS_ASSERT_EQUALS( el0.getNumberEvents(), 5);
    //And so on, the # of events = pixel ID
    const EventList el1(uneven->getEventList(1));
    TS_ASSERT_EQUALS( el1.getNumberEvents(), 15);
    const EventList el5(uneven->getEventList(5));
    TS_ASSERT_EQUALS( el5.getNumberEvents(), 55);

    //Out of range
    TS_ASSERT_THROWS( uneven->dataX(-3), std::range_error );
    TS_ASSERT_THROWS( uneven->dataX(NUMPIXELS/10), std::range_error );
  }

  //------------------------------------------------------------------------------
  void test_getEventListAtPixelID()
  {
    //Get pixel 1
    const EventList el(ew->getEventList(1));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA);
    //Because of the way the events were faked, bins 0 to pixel-1 are 0, rest are 1
    TS_ASSERT_EQUALS( (*el.dataY())[0], 0);
    TS_ASSERT_EQUALS( (*el.dataY())[1], 2);
    TS_ASSERT_EQUALS( (*el.dataY())[2], 2);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMEVENTS], 2);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMEVENTS+1], 0);
    //And some errors
    TS_ASSERT_DELTA( (*el.dataE())[0], 0, 1e-6);
    TS_ASSERT_DELTA( (*el.dataE())[1], sqrt(2.0), 1e-6);
    TS_ASSERT_DELTA( (*el.dataE())[2], sqrt(2.0), 1e-6);
  }

  //------------------------------------------------------------------------------
  void test_data_access()
  {
    //Non-const access throws errors
    TS_ASSERT_THROWS( ew->dataX(1), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataY(2), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataE(3), NotImplementedError );
    //Out of range
    TS_ASSERT_THROWS( ew->dataX(-123), std::range_error );
    TS_ASSERT_THROWS( ew->dataX(5123), std::range_error );
    TS_ASSERT_THROWS( ew->dataE(5123), std::range_error );
    TS_ASSERT_THROWS( ew->dataY(5123), std::range_error );

    //Can't try the const access; copy constructors are not allowed.
  }

  //------------------------------------------------------------------------------
  void test_data_access_not_setting_num_vectors()
  {
    ew = createEventWorkspace(0, 1);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), (NUMBINS-1));
    TS_ASSERT_EQUALS( ew->size(), (NUMBINS-1)*NUMPIXELS);
    TS_ASSERT_THROWS( ew->dataX(-123), std::range_error );
    TS_ASSERT_THROWS( ew->dataX(5123), std::range_error );
    //Non-const access throws errors, but not RANGE errors!
    TS_ASSERT_THROWS( ew->dataX(1), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataY(2), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataE(3), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataX(3), NotImplementedError );
  }

  //------------------------------------------------------------------------------
  void test_setX_individually()
  {
    //Create A DIFFERENT x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(NUMBINS/2);
    for (int i = 0; i < NUMBINS/2; ++i)
      xRef[i] = i*BIN_DELTA*2;

    ew->setX(0, axis);
    const EventList el(ew->getEventList(0));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA*2);

    //Are the returned arrays the right size?
    TS_ASSERT_EQUALS( el.dataX().size(), NUMBINS/2);
    TS_ASSERT_EQUALS( el.dataY()->size(), NUMBINS/2-1);
    TS_ASSERT_EQUALS( el.dataE()->size(), NUMBINS/2-1);

    //Now there are 4 events in each bin
    TS_ASSERT_EQUALS( (*el.dataY())[0], 4);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMEVENTS/2-1], 4);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMEVENTS/2], 0);

    //But pixel 1 is the same, 2 events in the bin
    const EventList el1(ew->getEventList(1));
    TS_ASSERT_EQUALS( el1.dataX()[1], BIN_DELTA*1);
    TS_ASSERT_EQUALS( (*el1.dataY())[1], 2);
  }


  void testIntegrateSpectra_entire_range()
  {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 0, 0, true);
    for (int i = 0; i < NUMPIXELS; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], (NUMBINS-1) * 2.0 );;
    }
  }
  void testIntegrateSpectra_empty_range()
  {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, false);
    for (int i = 0; i < NUMPIXELS; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], 0.0 );;
    }
  }

  void testIntegrateSpectra_partial_range()
  {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    //This will include a single bin
    ws->getIntegratedSpectra(sums, BIN_DELTA*1.9, BIN_DELTA*3.1, false);
    for (int i = 0; i < NUMPIXELS; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], 2 );;
    }
  }









  //------------------------------------------------------------------------------
  /// Linux-only method for getting memory usage
  int memory_usage()
  {
    // Linux only memory test
#ifdef _WIN32
    //Temporarily disabled for non-linux OSs
#else
    char buf[30];
    snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
    FILE* pf = fopen(buf, "r");
    if (pf) {
        int size; //       total program size
        fscanf(pf, "%u" /* %u %u %u %u %u"*/, &size/*, &resident, &share, &text, &lib, &data*/);
        fclose(pf);
        return size*4; //On my system each number here = 4 kb
    }
    fclose(pf);
#endif
    return 0;
  }

  //------------------------------------------------------------------------------
  void test_histogram_cache()
  {
    //Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 = boost::dynamic_pointer_cast<const EventWorkspace>(ew);
    //Are the returned arrays the right size?
    MantidVec data1 = ew2->dataY(1);
    TS_ASSERT_EQUALS( data1.size(), NUMBINS-1);
    //This should get the cached one
    MantidVec data2 = ew2->dataY(1);
    TS_ASSERT_EQUALS( data2.size(), NUMBINS-1);
    //All elements are the same
    for (int i=0; i<data1.size();i++)
      TS_ASSERT_EQUALS( data1[i], data2[i]);

    //Now test the caching. The first 100 will load in memory
    for (int i=0; i<100;i++)
      data1 = ew2->dataY(i);

    //Check the bins contain 2
    data1 = ew2->dataY(0);
    TS_ASSERT_DELTA( ew2->dataY(0)[1], 2.0, 1e-6);
    TS_ASSERT_DELTA( data1[1], 2.0, 1e-6);

    int mem1 = memory_usage();
    int mem2 = 0;
    int last = 100;
    //Read more; memory use should be the same?

    for (int i=last; i<last+100;i++)
      data1 = ew2->dataY(i);

#ifndef WIN32
    mem2 = memory_usage();
    TS_ASSERT_LESS_THAN( mem2-mem1, 10); //Memory usage should be ~the same.
#endif

    //Do it some more
    last=200; mem1=mem2;
    for (int i=last; i<last+100;i++)
      data1 = ew2->dataY(i);


#ifndef WIN32
    mem2 = memory_usage();
    TS_ASSERT_LESS_THAN( mem2-mem1, 10); //Memory usage should be ~the same.
#endif


    //----- Now we test that setAllX clears the memory ----
    mem1=mem2;

    //Yes, our eventworkspace MRU is full
    TS_ASSERT_EQUALS( ew->MRUSize(), 50);
    TS_ASSERT_EQUALS( ew2->MRUSize(), 50);
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(10);
    for (int i = 0; i < 10; ++i) xRef[i] = i*BIN_DELTA;
    ew->setAllX(axis);

    //MRU should have been cleared now
    TS_ASSERT_EQUALS( ew->MRUSize(), 0);
    TS_ASSERT_EQUALS( ew2->MRUSize(), 0);

//#ifndef WIN32
//    mem2 = memory_usage();
//    std::cout << "Mem change " << mem2-mem1 << "\n";
//    TS_ASSERT_LESS_THAN( mem2-mem1, 0); //Memory usage should be lower!.
//#endif

  }

  //------------------------------------------------------------------------------
  void test_histogram_cache_dataE()
  {
    //Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 = ew;
    //Are the returned arrays the right size?
    MantidVec data1 = ew2->dataE(1);
    TS_ASSERT_EQUALS( data1.size(), NUMBINS-1);
    //This should get the cached one
    MantidVec data2 = ew2->dataE(1);
    TS_ASSERT_EQUALS( data2.size(), NUMBINS-1);
    //All elements are the same
    for (int i=0; i<data1.size();i++)
      TS_ASSERT_EQUALS( data1[i], data2[i]);

    //Now test the caching. The first 100 will load in memory
    for (int i=0; i<100;i++)
      data1 = ew2->dataE(i);

    //Check the bins contain 2
    data1 = ew2->dataE(0);
    TS_ASSERT_DELTA( ew2->dataE(0)[1], sqrt(2.0), 1e-6);
    TS_ASSERT_DELTA( data1[1], sqrt(2.0), 1e-6);
    //But the Y is still 2.0
    TS_ASSERT_DELTA( ew2->dataY(0)[1], 2.0, 1e-6);

    int mem1 = memory_usage();
    int mem2 = 0;
    int last = 100;
    //Read more; memory use should be the same?
    for (int i=last; i<last+100;i++)
      data1 = ew2->dataE(i);

//#ifndef WIN32
//    mem2 = memory_usage();
//    TS_ASSERT_LESS_THAN( mem2-mem1, 10); //Memory usage should be ~the same.
//#endif

    //Do it some more
    last=200; mem1=mem2;
    for (int i=last; i<last+100;i++)
      data1 = ew2->dataE(i);

//#ifndef WIN32
//    mem2 = memory_usage();
//    TS_ASSERT_LESS_THAN( mem2-mem1, 10); //Memory usage should be ~the same.
//#endif

  }




  void testSortByTof()
  {
    EventWorkspace_sptr test_in = CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    Progress * prog = NULL;

    test_in->sortAll(TOF_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), NUMBINS);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].tof(), ve[i+1].tof());

  }



  /** Test sortAll() when there are more cores available than pixels.
   * This test will only work on machines with 2 cores at least.
   */
  void testSortAll_Parallel()
  {
    int numEvents = 30;
    EventWorkspace_sptr test_in = CreateRandomEventWorkspace(numEvents, 1);
    Progress * prog = NULL;

    test_in->sortAll(TOF_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), numEvents);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].tof(), ve[i+1].tof());
  }



  void testSortByPulseTime()
  {
    EventWorkspace_sptr test_in = CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    Progress * prog = NULL;

    test_in->sortAll(PULSETIME_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), NUMBINS);
     for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].pulseTime(), ve[i+1].pulseTime());
  }


  /** Nov 29 2010, ticket #1974
   * SegFault on data access through MRU list.
   * Test that parallelization is thread-safe
   */
  void xtestSegFault()
  {
    //std::cout << omp_get_num_threads() << " threads available\n";
    //std::cout << " Starting testSegFault 1 \n\n";    std::cout.flush();

    int numpix = 100000;
    EventWorkspace_const_sptr ew1 = CreateRandomEventWorkspace(50, numpix);

    //std::cout << " Starting testSegFault 2 \n\n";    std::cout.flush();
    //#pragma omp parallel for
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < numpix; i++)
    {
      //std::cout << "Thread " << PARALLEL_THREAD_NUMBER << " is starting with " << i << "\n";
      for (int j=0; j < 10; j++)
      {
        MantidVec Y = ew1->dataY(i);
        const MantidVec & E = ew1->dataE(i);
        MantidVec E2 = E;
      }
      //std::cout << "Thread " << PARALLEL_THREAD_NUMBER << " should be done with " << i << "\n";
    }
  }





};

#endif /* EVENTWORKSPACETEST_H_ */

/*
 * EventWorkspaceTest.h
 *
 *  Created on: May 28, 2010
 *      Author: Janik Zikovsky
 */

#ifndef EVENTWORKSPACETEST_H_
#define EVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
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
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventWorkspaceTest *createSuite() { return new EventWorkspaceTest(); }
  static void destroySuite( EventWorkspaceTest *suite ) { delete suite; }

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
  EventWorkspace_sptr createEventWorkspace(bool initialize_pixels, bool setX, bool evenTOFs=false)
  {

    EventWorkspace_sptr retVal(new EventWorkspace);
    if (initialize_pixels)
    {
      retVal->initialize(NUMPIXELS,1,1);

      //Make fake events
      for (int pix=0; pix<NUMPIXELS; pix++)
      {
        for (int i=0; i<NUMBINS-1; i++)
        {
          double tof = 0;
          if (evenTOFs)
          {
            tof = (i+0.5)*BIN_DELTA;
          }
          else
          {
            //Two events per bin
            tof = (pix+i+0.5)*BIN_DELTA;
          }
          size_t pulse_time = static_cast<size_t>(tof);
          retVal->getEventList(pix) += TofEvent(tof, pulse_time);
          retVal->getEventList(pix) += TofEvent(tof, pulse_time);
        }
        retVal->getEventList(pix).addDetectorID(pix);
        retVal->getEventList(pix).setSpectrumNo(pix);
      }
    }
    else
    {
      retVal->initialize(1,1,1);
    }

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
    return createEventWorkspace(1,1, true);
//    EventWorkspace_sptr retVal(new EventWorkspace);
//    retVal->initialize(NUMPIXELS,1,1);
//    //Make fake eventscreateEventWorkspace
//    for (int pix=0; pix<NUMPIXELS; pix++)
//    {
//      for (int i=0; i<NUMBINS-1; i++)
//      {
//        //Two events per bin
//        retVal->getEventList(pix) += TofEvent((i+0.5)*BIN_DELTA, 1);
//        retVal->getEventList(pix) += TofEvent((i+0.5)*BIN_DELTA, 1);
//      }
//      retVal->getEventList(pix).addDetectorID(pix);
//      retVal->getEventList(pix).setSpectrumNo(pix);
//    }
//
//    //Create the x-axis for histogramming.
//    Kernel::cow_ptr<MantidVec> axis;
//    MantidVec& xRef = axis.access();
//    xRef.resize(NUMBINS);
//    for (int i = 0; i < NUMBINS; ++i)
//      xRef[i] = i*BIN_DELTA;
//    //Set all the histograms at once.
//    retVal->setAllX(axis);
//    return retVal;
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
    TS_ASSERT_EQUALS( el.constDataX().size(), NUMBINS);
    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS( Y->size(), NUMBINS-1);
    TS_ASSERT_EQUALS( E->size(), NUMBINS-1);
    TS_ASSERT( el.hasDetectorID(1) );
  }

  void test_copyDataFrom()
  {
    EventWorkspace_sptr ew1 = createFlatEventWorkspace();
    TS_ASSERT_DELTA( ew1->readY(0)[0], 2.0, 1e-5);
    TS_ASSERT_DELTA( ew1->readY(1)[0], 2.0, 1e-5);

    EventWorkspace_sptr ew2(new EventWorkspace);
    ew2->initialize(2, 2, 2);
    ew2->copyDataFrom(*ew1);
    TS_ASSERT_EQUALS( ew2->getNumberHistograms(), ew1->getNumberHistograms() );
    TS_ASSERT_EQUALS( ew2->getNumberEvents(), ew1->getNumberEvents() );

    // Double # of events in the copied workspace
    ew2->getEventList(0) += ew2->getEventList(0);
    ew2->getEventList(1) += ew2->getEventList(1);

    // Original is still 2.0
    TS_ASSERT_DELTA( ew1->readY(0)[0], 2.0, 1e-5);
    TS_ASSERT_DELTA( ew1->readY(1)[0], 2.0, 1e-5);
    // New one is 4.0
    TSM_ASSERT_DELTA("Copied workspace's Y values are properly refreshed thanks to a correct MRU.", ew2->readY(0)[0], 4.0, 1e-5);
    TSM_ASSERT_DELTA("Copied workspace's Y values are properly refreshed thanks to a correct MRU.", ew2->readY(1)[0], 4.0, 1e-5);


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

    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), 1023+1);

  }

  //------------------------------------------------------------------------------
  void test_getMemorySize()
  {
    // Because of the way vectors allocate, we can only know the minimum amount of memory that can be used.
    size_t min_memory = (ew->getNumberEvents() * sizeof(TofEvent) + NUMPIXELS * sizeof(EventList));
    TS_ASSERT_LESS_THAN_EQUALS(min_memory,  ew->getMemorySize());
  }

  //------------------------------------------------------------------------------
  void test_destructor()
  {
    EventWorkspace * ew2 = new EventWorkspace();
    delete ew2;
  }


  //------------------------------------------------------------------------------
  void test_constructor_setting_default_x()
  {
    //Do the workspace, but don't set x explicity
    ew = createEventWorkspace(1, 0);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), 1);
    TS_ASSERT_EQUALS( ew->size(), 500);

    //Didn't set X? well all the histograms show a single bin
    const EventList el(ew->getEventList(1));
    TS_ASSERT_EQUALS( el.constDataX().size(), 2);
    TS_ASSERT_EQUALS( el.constDataX()[0], 0.0);
    TS_ASSERT_EQUALS( el.constDataX()[1], std::numeric_limits<double>::min());
    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    TS_ASSERT_EQUALS( Y->size(), 1);
    TS_ASSERT_EQUALS( (*Y)[0], 0.0);
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS( E->size(), 1);
    TS_ASSERT_EQUALS( (*E)[0], 0.0);
  }

  void test_maskWorkspaceIndex()
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10, false /*dont clear the events*/);
    TS_ASSERT_EQUALS( ws->getEventList(2).getNumberEvents(), 200);
    ws->maskWorkspaceIndex(2);
    TS_ASSERT_EQUALS( ws->getEventList(2).getNumberEvents(), 0);
  }

  void test_resizeTo()
  {
    ew = createEventWorkspace(false, false);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), 1 );
    ew->resizeTo(3);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), 3 );
    for ( size_t i = 0; i < ew->getNumberHistograms(); ++i )
    {
      TS_ASSERT_EQUALS( ew->getSpectrum(i)->getSpectrumNo(), i+1 );
      //TS_ASSERT( ew->getEventList(i).empty() );
      TS_ASSERT_EQUALS( ew->readX(i).size(), 2);
    }
  }

  //------------------------------------------------------------------------------
  void test_padSpectra()
  {
    bool timing = false;
    ew = createEventWorkspace(true, false);

    int numpixels = timing ? 900000 : 1800;
    //Make an instrument with lots of pixels
    ew->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(numpixels/9));

    Timer timer;
    ew->padSpectra();
    if (timing) std::cout << "\n" << timer.elapsed() << " seconds for padSpectra().\n";

    TS_ASSERT_EQUALS(ew->getNumberHistograms(), numpixels);
    int badcount = 0;
    for (int i=0; i < numpixels; i++)
    {
      ISpectrum * spec = ew->getSpectrum(i);
      bool b = spec->hasDetectorID(i+1);
      TSM_ASSERT("Workspace i has the given detector id i+1", b);
      TSM_ASSERT_EQUALS("Matching detector ID and spectrum number.", spec->getSpectrumNo(), i+1);
      if (b)
        if (badcount++ > 40) break;
    }

  }



  //------------------------------------------------------------------------------
  void test_uneven_pixel_ids()
  {
    EventWorkspace_sptr uneven(new EventWorkspace);
    uneven->initialize(NUMPIXELS/10,1,1);

    //Make fake events. Pixel IDs start at 5 increment by 10
    size_t wi = 0;
    for (int pix=5; pix<NUMPIXELS; pix += 10)
    {
      for (int i=0; i<pix; i++)
      {
        uneven->getEventList(wi) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
      }
      uneven->getEventList(wi).addDetectorID(pix);
      uneven->getEventList(wi).setSpectrumNo(pix);
      wi++;
    }

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
    TS_ASSERT_EQUALS( uneven->getAxis(1)->spectraNo(0), 5 );
    TS_ASSERT_EQUALS( uneven->getAxis(1)->spectraNo(5), 55 );
    TS_ASSERT_EQUALS( uneven->getAxis(1)->length(), NUMPIXELS/10 );

    //The spectra map should take each workspace index and point to the right pixel id: 5,15,25, etc.
    for (int wi=0; wi<static_cast<int>(uneven->getNumberHistograms()); wi++)
    {
      TS_ASSERT_EQUALS( *uneven->getSpectrum(wi)->getDetectorIDs().begin(), 5 + wi*10);
    }

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
  void test_data_access()
  {
    //Non-const access throws errors for Y & E - not for X
    TS_ASSERT_THROWS_NOTHING( ew->dataX(1) );
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
    TS_ASSERT_EQUALS( el.constDataX()[0], 0);
    TS_ASSERT_EQUALS( el.constDataX()[1], BIN_DELTA*2);

    //Are the returned arrays the right size?
    TS_ASSERT_EQUALS( el.constDataX().size(), NUMBINS/2);

    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS( Y->size(), NUMBINS/2-1);
    TS_ASSERT_EQUALS( E->size(), NUMBINS/2-1);

    //Now there are 4 events in each bin
    TS_ASSERT_EQUALS( (*Y)[0], 4);
    TS_ASSERT_EQUALS( (*Y)[NUMBINS/2-2], 4);

    //But pixel 1 is the same, 2 events in the bin
    const EventList el1(ew->getEventList(1));
    TS_ASSERT_EQUALS( el1.constDataX()[1], BIN_DELTA*1);
    boost::scoped_ptr<MantidVec> Y1(el1.makeDataY());
    TS_ASSERT_EQUALS( (*Y1)[1], 2);
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
        int status = fscanf(pf, "%u" /* %u %u %u %u %u"*/, &size/*, &resident, &share, &text, &lib, &data*/);
        (void) status;
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
    for (std::size_t i=0; i<data1.size();i++)
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
    for (std::size_t i=0; i<data1.size();i++)
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

    int last = 100;
    //Read more; memory use should be the same?
    for (int i=last; i<last+100;i++)
      data1 = ew2->dataE(i);

    //Do it some more
    last=200;
    for (int i=last; i<last+100;i++)
      data1 = ew2->dataE(i);
  }

  void test_histogram_pulse_time_throws_if_index_too_large()
  {
    const size_t nHistos = 10;
    EventWorkspace_sptr ws = boost::make_shared<EventWorkspace>(); 
    ws->initialize(nHistos, 1, 1);

    MantidVec X, Y, E;
    TSM_ASSERT_THROWS("Number of histograms is out of range, should throw", ws->generateHistogramPulseTime(nHistos+1, X, Y, E), std::range_error);
  }

  void do_test_binning(EventWorkspace_sptr ws, const MantidVec& X, cow_ptr<MantidVec> axis, size_t expected_occupancy_per_bin)
  {
    MantidVec Y(NUMBINS-1);
    MantidVec E(NUMBINS-1);
    // Required since we are rebinning in place.
    ws->setAllX(axis);
    // perform binning 
    ws->generateHistogramPulseTime(0, X, Y, E);
    // Check results
    for(size_t j = 0; j < Y.size(); ++j)
    {
      TS_ASSERT_EQUALS(expected_occupancy_per_bin, Y[j]); 
    }
  }

  void test_histogram_pulse_time()
  {
    const size_t nHistos = 1;
    const bool setXAxis = false;
    EventWorkspace_sptr ws = createEventWorkspace(nHistos, setXAxis); // Creates TOF events with pulse_time intervals of BIN_DELTA/2

    Mantid::Kernel::cow_ptr<MantidVec> axis;
    MantidVec& X = axis.access();
    
    // Create bin steps = 4*BIN_DELTA.
    X.resize(NUMBINS/4);
    for (size_t i = 0; i < X.size(); ++i)
    {
      X[i] = double(i)*BIN_DELTA*4;
    }
    size_t expected_occupancy = 8; // Because there are two events with pulse_time in each BIN_DELTA interval. 
    do_test_binning(ws, X, axis, expected_occupancy);
   
    // Create bin steps = 2*BIN_DELTA.
    X.clear();
    X.resize(NUMBINS/2);
    for (size_t i = 0; i < X.size(); ++i)
    {
      X[i] = double(i)*BIN_DELTA*2;
    }
    expected_occupancy = 4; // Because there are two events with pulse_time in each BIN_DELTA interval. 
    do_test_binning(ws, X, axis, expected_occupancy);

    // Create bin steps = BIN_DELTA.
    X.clear();
    X.resize(NUMBINS);
    for (size_t i = 0; i < X.size(); ++i)
    {
      X[i] = double(i)*BIN_DELTA;
    }
    expected_occupancy = 2; // Because there are two events with pulse_time in each BIN_DELTA interval. 
    do_test_binning(ws, X, axis, expected_occupancy);

  }

  void test_get_pulse_time_max()
  {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1,2,1);
    ws->getEventList(0) += TofEvent(0, min); // min
    ws->getEventList(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(max, ws->getPulseTimeMax());
  }

  void test_get_pulse_time_min()
  {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1,2,1);
    ws->getEventList(0) += TofEvent(0, min); // min
    ws->getEventList(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(min, ws->getPulseTimeMin());
  }


  void test_get_time_at_sample_max_min_with_colocated_detectors()
  {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(4);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(2,2,1);
    // First spectrum
    ws->getEventList(0) += TofEvent(0, min + int64_t(1));
    ws->getEventList(0) += TofEvent(0, max); // max in spectra 1
    // Second spectrum
    ws->getEventList(1) += TofEvent(0, min); // min in spectra 2
    ws->getEventList(1) += TofEvent(0, max - int64_t(1));


    V3D source(0,0,0);
    V3D sample(10,0,0);
    std::vector<V3D> detectorPositions;

    detectorPositions.push_back(V3D(11,1,0)); // First detector pos
    detectorPositions.push_back(V3D(11,1,0)); // Second detector sits on the first.

    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(ws, source, sample, detectorPositions);

    DateAndTime foundMin = ws->getTimeAtSampleMin();
    DateAndTime foundMax = ws->getTimeAtSampleMax();

    TS_ASSERT_EQUALS(max, foundMax);
    TS_ASSERT_EQUALS(min, foundMin);
  }

  void test_get_time_at_sample_min()
  {
    /*
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1,2,1);
    ws->getEventList(0) += TofEvent(0, min); // min
    ws->getEventList(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(min, ws->getPulseTimeMin());
    */
  }


  //------------------------------------------------------------------------------
  void test_droppingOffMRU()
  {
    //Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 = boost::dynamic_pointer_cast<const EventWorkspace>(ew);

    // OK, we grab data0 from the MRU.
    const ISpectrum * inSpec = ew2->getSpectrum(0);
    const ISpectrum * inSpec300 = ew2->getSpectrum(300);
    inSpec->lockData();
    inSpec300->lockData();

    const MantidVec & data0 = inSpec->readY();
    const MantidVec & e300 = inSpec300->readE();
    TS_ASSERT_EQUALS( data0.size(), NUMBINS-1);
    MantidVec data0_copy(data0);
    MantidVec e300_copy(e300);


    // Fill up the MRU to make data0 drop off
    for (size_t i=0; i<200; i++)
      MantidVec otherData = ew2->readY(i);

    // data0 should not have changed!
    for (size_t i=0; i<data0.size(); i++)
    { TS_ASSERT_EQUALS( data0[i], data0_copy[i] ); }

    for (size_t i=0; i<e300.size(); i++)
    { TS_ASSERT_EQUALS( e300[i], e300_copy[i] ); }

    inSpec->unlockData();
    inSpec300->unlockData();

    MantidVec otherData = ew2->readY(255);

    // MRU is full
    TS_ASSERT_EQUALS( ew2->MRUSize(), 50);
  }

  //------------------------------------------------------------------------------
  void test_sortAll_TOF()
  {
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    Progress * prog = NULL;

    test_in->sortAll(TOF_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    for (int wi=0; wi<NUMPIXELS; wi++)
    {
      std::vector<TofEvent> ve = outWS->getEventList(wi).getEvents();
      TS_ASSERT_EQUALS( ve.size(), NUMBINS);
       for (size_t i=0; i<ve.size()-1; i++)
          TS_ASSERT_LESS_THAN_EQUALS( ve[i].tof(), ve[i+1].tof());
    }
  }



  /** Test sortAll() when there are more cores available than pixels.
   * This test will only work on machines with 2 cores at least.
   */
  void test_sortAll_SingleEventList()
  {
    int numEvents = 30;
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(numEvents, 1);
    Progress * prog = NULL;

    test_in->sortAll(TOF_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), numEvents);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].tof(), ve[i+1].tof());
  }


  /** Test sortAll() when there are more cores available than pixels.
   * This test will only work on machines with 2 cores at least.
   */
  void test_sortAll_byTime_SingleEventList()
  {
    int numEvents = 30;
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(numEvents, 1);
    Progress * prog = NULL;

    test_in->sortAll(PULSETIME_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getEventList(0).getEvents();
    TS_ASSERT_EQUALS( ve.size(), numEvents);
    for (size_t i=0; i<ve.size()-1; i++)
      TS_ASSERT_LESS_THAN_EQUALS( ve[i].pulseTime(), ve[i+1].pulseTime());
  }


  void test_sortAll_ByTime()
  {
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateRandomEventWorkspace(NUMBINS, NUMPIXELS);
    Progress * prog = NULL;

    test_in->sortAll(PULSETIME_SORT, prog);

    EventWorkspace_sptr outWS = test_in;
    for (int wi=0; wi<NUMPIXELS; wi++)
    {
      std::vector<TofEvent> ve = outWS->getEventList(wi).getEvents();
      TS_ASSERT_EQUALS( ve.size(), NUMBINS);
       for (size_t i=0; i<ve.size()-1; i++)
        TS_ASSERT_LESS_THAN_EQUALS( ve[i].pulseTime(), ve[i+1].pulseTime());
    }
  }




  /** Nov 29 2010, ticket #1974
   * SegFault on data access through MRU list.
   * Test that parallelization is thread-safe
   *
   */
  void xtestSegFault() ///<Disabled because ~2.5 seconds.
  {
    int numpix = 100000;
    EventWorkspace_const_sptr ew1 = WorkspaceCreationHelper::CreateRandomEventWorkspace(50, numpix);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < numpix; i++)
    {
      for (int j=0; j < 10; j++)
      {
        MantidVec Y = ew1->dataY(i);
        const MantidVec & E = ew1->dataE(i);
        MantidVec E2 = E;
      }
    }
  }


  /** Refs #2649: Add a dirty flag when changing X on an event list */
  void do_test_dirtyFlag(bool do_parallel)
  {
    // 50 pixels, 100 bins, 2 events in each
    int numpixels = 900;
    EventWorkspace_sptr ew1 = WorkspaceCreationHelper::CreateEventWorkspace2(numpixels, 100);
    PARALLEL_FOR_IF(do_parallel)
    for (int i=0; i < numpixels; i += 3)
    {
      const MantidVec & Y = ew1->readY(i);
      TS_ASSERT_DELTA( Y[0], 2.0, 1e-5);
      const MantidVec & E = ew1->readE(i);
      TS_ASSERT_DELTA( E[0], sqrt(2.0), 1e-5);

      // Vector with 10 bins, 10 wide
      MantidVec X;
      for (size_t j=0; j<11; j++)
        X.push_back(double(j)*10.0);
      ew1->setX(i, X);

      // Now it should be 20 in that spot
      const MantidVec & Y_now = ew1->readY(i);
      TS_ASSERT_DELTA( Y_now[0], 20.0, 1e-5);
      const MantidVec & E_now = ew1->readE(i);
      TS_ASSERT_DELTA( E_now[0], sqrt(20.0), 1e-5);

      // But the other pixel is still 2.0
      const MantidVec & Y_other = ew1->readY(i+1);
      TS_ASSERT_DELTA( Y_other[0], 2.0, 1e-5);
      const MantidVec & E_other = ew1->readE(i+1);
      TS_ASSERT_DELTA( E_other[0], sqrt(2.0), 1e-5);
    }
  }

  void test_dirtyFlag()
  {
    do_test_dirtyFlag(false);
  }

  void test_dirtyFlag_parallel()
  {
    do_test_dirtyFlag(true);
  }

  void test_getEventXMinMax()
  {
    EventWorkspace_sptr wksp = createEventWorkspace(true, true, true);
    TS_ASSERT_DELTA(wksp->getEventXMin(), 500, .01);
    TS_ASSERT_DELTA(wksp->getEventXMax(), 1023500, .01);
  }

};

#endif /* EVENTWORKSPACETEST_H_ */

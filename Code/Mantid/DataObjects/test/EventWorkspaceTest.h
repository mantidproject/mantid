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
    NUMBINS = 1001;
    NUMEVENTS = 100;
    BIN_DELTA = 1e3;
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
        retVal->getEventList(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
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

        //Try setting a single axis
        retVal->setX(2, axis);

        //Set all the histograms at once.
        retVal->setAllX(axis);
      }

    return retVal;
  }

  void setUp()
  {
    ew = createEventWorkspace(1, 1);
  }


  //------------------------------------------------------------------------------
  void test_constructor()
  {
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), NUMBINS);
    TS_ASSERT_EQUALS( ew->size(), NUMBINS*NUMPIXELS);


    //Are the returned arrays the right size?
    const EventList el(ew->getEventListAtWorkspaceIndex(1));
    TS_ASSERT_EQUALS( el.dataX().size(), NUMBINS);
    TS_ASSERT_EQUALS( el.dataY().size(), NUMBINS);
    TS_ASSERT_EQUALS( el.dataE().size(), NUMBINS);

  }


  //------------------------------------------------------------------------------
  void test_constructor_not_settingx()
  {
    //Do the workspace, but don't set x
    ew = createEventWorkspace(1, 0);
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), 1);
    TS_ASSERT_EQUALS( ew->size(), 1*NUMPIXELS);

    //Didn't set X? well the default bin = everything
    const EventList el(ew->getEventListAtWorkspaceIndex(1));
    TS_ASSERT_EQUALS( el.dataX().size(), 0);
    TS_ASSERT_EQUALS( el.dataY().size(), 1);
    TS_ASSERT_EQUALS( el.dataE().size(), 1);
    TS_ASSERT_EQUALS( el.dataY()[0], NUMEVENTS);

  }


  //------------------------------------------------------------------------------
  void test_uneven_pixel_ids()
  {
    EventWorkspace_sptr uneven(new EventWorkspace);
    uneven->initialize(1,1,1);

    //Make fake events. Spectrum IDs start at 5 increment by 10
    for (int pix=5; pix<NUMPIXELS; pix += 10)
    {
      for (int i=0; i<pix; i++)
      {
        uneven->getEventList(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
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
    TS_ASSERT_EQUALS( uneven->blocksize(), NUMBINS);
    TS_ASSERT_EQUALS( uneven->size(), NUMBINS*NUMPIXELS/10);

    //Does the spectra map make sense
    TS_ASSERT_EQUALS( uneven->spectraMap().getDetectors(0)[0], 5);
    TS_ASSERT_EQUALS( uneven->spectraMap().getDetectors(5)[0], 55);

    //Spectrum 0 is at pixelid 5 and has 5 events
    const EventList el0(uneven->getEventListAtWorkspaceIndex(0));
    TS_ASSERT_EQUALS( el0.getNumberEvents(), 5);
    const EventList el1(uneven->getEventListAtWorkspaceIndex(1));
    TS_ASSERT_EQUALS( el1.getNumberEvents(), 15);
    const EventList el5(uneven->getEventListAtWorkspaceIndex(5));
    TS_ASSERT_EQUALS( el5.getNumberEvents(), 55);

    //Out of range
    TS_ASSERT_THROWS( uneven->dataX(-3), std::range_error );
    TS_ASSERT_THROWS( uneven->dataX(NUMPIXELS/10), std::range_error );
  }

  //------------------------------------------------------------------------------
  void test_getEventList()
  {
    //Get pixel 1
    const EventList el(ew->getEventListAtWorkspaceIndex(1));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA);
    //Because of the way the events were faked, bins 0 to pixel-1 are 0, rest are 1
    TS_ASSERT_EQUALS( el.dataY()[0], 0);
    TS_ASSERT_EQUALS( el.dataY()[1], 1);
    TS_ASSERT_EQUALS( el.dataY()[2], 1);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS], 1);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS+1], 0);
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
    TS_ASSERT_EQUALS( ew->blocksize(), NUMBINS);
    TS_ASSERT_EQUALS( ew->size(), NUMBINS*NUMPIXELS);
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
    const EventList el(ew->getEventListAtWorkspaceIndex(0));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA*2);

    //Are the returned arrays the right size?
    TS_ASSERT_EQUALS( el.dataX().size(), NUMBINS/2);
    TS_ASSERT_EQUALS( el.dataY().size(), NUMBINS/2);
    TS_ASSERT_EQUALS( el.dataE().size(), NUMBINS/2);

    //Now there are 2 events in each bin
    TS_ASSERT_EQUALS( el.dataY()[0], 2);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS/2-1], 2);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS/2], 0);

    //But pixel 1 is the same
    const EventList el1(ew->getEventListAtWorkspaceIndex(1));
    TS_ASSERT_EQUALS( el1.dataX()[1], BIN_DELTA*1);
    TS_ASSERT_EQUALS( el1.dataY()[1], 1);
  }

  //------------------------------------------------------------------------------
  void test_frameTime()
  {
    //Nothing yet
    TS_ASSERT_THROWS(ew->getTime(0), std::range_error);
    //Add some times
    ptime t;
    t = microsec_clock::local_time();
    ew->addTime(0, t );
    TS_ASSERT_EQUALS(ew->getTime(0), t);
    //Add another id
    ew->addTime(1000, t+ minutes(5) );
    TS_ASSERT_EQUALS(ew->getTime(1000), t+minutes(5));
    //Intermediate ones are not-a-date
    TS_ASSERT(ew->getTime(234).is_not_a_date_time());

    //Invalid addition - doesn't work because -100 gets wrapped back to positive because of size_t
    //TS_ASSERT_THROWS( ew->addTime(-100, t - minutes(5) ), std::range_error);
  }

  //------------------------------------------------------------------------------
  void test_histogram_cache()
  {
	  //Try caching and most-recently-used MRU list.
  }



};




#endif /* EVENTWORKSPACETEST_H_ */

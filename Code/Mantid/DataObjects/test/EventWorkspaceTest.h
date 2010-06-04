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

const int NUMPIXELS = 500;
const int NUMBINS = 1001;
const int NUMEVENTS = 100;
const double BIN_DELTA = 1e3;


//==========================================================================================
class EventWorkspaceTest : public CxxTest::TestSuite
{
private:
  EventWorkspace_sptr ew;

public:
  EventWorkspaceTest()
  {
  }


  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  EventWorkspace_sptr createEventWorkspace()
  {

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(NUMPIXELS,1,1);

    //Make fake events
    for (int pix=0; pix<NUMPIXELS; pix++)
    {
      for (int i=0; i<NUMEVENTS; i++)
      {
        retVal->getEventList(pix) += TofEvent((pix+i+0.5)*BIN_DELTA, 1);
      }
    }

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

    return retVal;
  }

  void setUp()
  {
    ew = createEventWorkspace();
  }

  void test_constructor()
  {
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS( ew->blocksize(), NUMBINS);
    TS_ASSERT_EQUALS( ew->size(), NUMBINS*NUMPIXELS);
  }

  void test_getEventList()
  {
    //Get pixel 1
    const EventList el(ew->getEventList(1));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA);
    //Because of the way the events were faked, bins 0 to pixel-1 are 0, rest are 1
    TS_ASSERT_EQUALS( el.dataY()[0], 0);
    TS_ASSERT_EQUALS( el.dataY()[1], 1);
    TS_ASSERT_EQUALS( el.dataY()[2], 1);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS], 1);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS+1], 0);
  }

  void test_data_access()
  {
    //Non-const access throws errors
    TS_ASSERT_THROWS( ew->dataX(1), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataY(2), NotImplementedError );
    TS_ASSERT_THROWS( ew->dataE(3), NotImplementedError );
    //Out of range
    TS_ASSERT_THROWS( ew->dataX(-123), std::range_error );
    TS_ASSERT_THROWS( ew->dataX(5123), std::range_error );

    //Can't try the const access; copy constructors are not allowed.
  }

  void test_setX_individually()
  {
    //Create A DIFFERENT x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(NUMBINS);
    for (int i = 0; i < NUMBINS/2; ++i)
      xRef[i] = i*BIN_DELTA*2;

    ew->setX(0, axis);
    const EventList el(ew->getEventList(0));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA*2);
    //Now there are 2 events in each bin
    TS_ASSERT_EQUALS( el.dataY()[0], 2);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS/2-1], 2);
    TS_ASSERT_EQUALS( el.dataY()[NUMEVENTS/2], 0);

    //But pixel 1 is the same
    const EventList el1(ew->getEventList(1));
    TS_ASSERT_EQUALS( el1.dataX()[1], BIN_DELTA*1);
    TS_ASSERT_EQUALS( el1.dataY()[1], 1);
  }

};




#endif /* EVENTWORKSPACETEST_H_ */

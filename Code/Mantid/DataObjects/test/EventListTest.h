#ifndef EVENTLISTTEST_H_
#define EVENTLISTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using std::runtime_error;
using std::size_t;
using std::vector;

//==========================================================================================
class TofEventTest : public CxxTest::TestSuite
{
private:
  TofEvent e;

public:
  TofEventTest()
  {
    e = TofEvent(123, 456);
  }

  void testInit()
  {
    TS_ASSERT_EQUALS(e.tof(), 123);
    TS_ASSERT_EQUALS(e.pulseTime(), 456);
  }

  void testAssign()
  {
    TofEvent e2;
    e2 = e;
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);
  }

  void testConstructors()
  {
    TofEvent e2 = TofEvent(e);
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);

    TofEvent e3 = TofEvent(890.234, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890.234);
    TS_ASSERT_EQUALS(e3.pulseTime(), 321);
  }

  void test_timestamp()
  {
    //Make an event workspace
    //EventWorkspace ews =

  }



//  void testBadInputs()
//  {
//    e = TofEvent(-100,1);
//    // SHOULD THIS BE THE BEHAVIOR FOR BAD INPUTS????
//    TS_ASSERT_EQUALS(e.tof(), 0);
//
//    e = TofEvent(1,-500);
//    // SHOULD THIS BE THE BEHAVIOR FOR BAD INPUTS????
//    TS_ASSERT_EQUALS(e.pulseTime(), 0);
//  }
};


//==========================================================================================
class EventListTest : public CxxTest::TestSuite
{
private:
  EventList el;
  static const int NUMEVENTS = 200;
  static const int NUMBINS = 1600;
  int BIN_DELTA;


public:
  EventListTest()
  {
    BIN_DELTA = 10000;
  }

  void setUp()
  {
    vector<TofEvent> mylist;
    mylist.push_back(TofEvent(100,200));
    mylist.push_back(TofEvent(3.5, 400));
    mylist.push_back(TofEvent(50,60));
    el = EventList(mylist);
  }


  void testInit()
  {
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 100);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].tof(), 50);
  }

  void testPlusOperator()
  {
    vector<TofEvent> mylist;
    mylist.push_back(TofEvent(45,67));
    mylist.push_back(TofEvent(89,12));
    mylist.push_back(TofEvent(34,56));
    el += mylist;
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 45);
    TS_ASSERT_EQUALS(rel[5].tof(), 34);

    el += TofEvent(999, 888);
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 7);
    TS_ASSERT_EQUALS(rel[6].tof(), 999);

    EventList el2;
    el2 += TofEvent(1,2);
    el2 += TofEvent(3,4);
    el += el2;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 9);
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 18);

    el.addEventQuickly( TofEvent(333, 444));
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 19);
  }

  void testPlusOperator2()
  {
    vector<TofEvent> rel;
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 100);
    TS_ASSERT_EQUALS(rel[5].tof(), 50);
  }

  void testDetectorIDs()
  {
    EventList el1;
    el1.addDetectorID( 14 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 1);
    el1.addDetectorID( 21 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 2);
    el1.addDetectorID( 21 );
    TS_ASSERT_EQUALS( el1.getDetectorIDs().size(), 2);

    EventList el2;
    el2.addDetectorID( 7 );
    el2.addDetectorID( 14 );
    el2.addDetectorID( 28 );
    TS_ASSERT_EQUALS( el2.getDetectorIDs().size(), 3);

    //One detID was repeated, so it doesn't appear twice
    el2 += el1;
    TS_ASSERT_EQUALS( el2.getDetectorIDs().size(), 4);
    //Find the right stuff
    for (int i=7; i< 35; i+=7)
      TS_ASSERT( el2.hasDetectorID(i) );
    TS_ASSERT( !el2.hasDetectorID(0) );

  }

  //==================================================================================
  //--- Sorting Tests ---

  void fake_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (int i=0; i < NUMEVENTS; i++)
    {
      //Random tof up to 10 ms
      //Random pulse time up to 1000
      el += TofEvent( 1e7*(rand()*1.0/RAND_MAX), rand()%1000);
    }
  }

  void testSortTOF()
  {
    el.sortTof();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 3.5);
    TS_ASSERT_EQUALS(rel[1].tof(), 50);
    TS_ASSERT_EQUALS(rel[2].tof(), 100);

    this->fake_data();
    el.sort(TOF_SORT);
    rel = el.getEvents();
    int i;
    for (i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rel[i-1].tof(), rel[i].tof());
    }
  }

  void testSortPulseTime()
  {
    el.sortPulseTime();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 60);
    TS_ASSERT_EQUALS(rel[1].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].pulseTime(), 400);

    this->fake_data();
    el.sort(PULSETIME_SORT);
    rel = el.getEvents();
    int i;
    for (i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rel[i-1].pulseTime(), rel[i].pulseTime());
    }
  }


  //==================================================================================
  //--- Histogramming Tests ---

  void test_setX()
  {
    //Generate the histrogram bins
    MantidVec shared_x;
    double tof; //in ns
    for (tof=0; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA)
    {
      //bins of 10 microsec
      shared_x.push_back(tof);
    }
    el.setX(shared_x);
    //Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.dataX()==shared_x);
  }


  void test_setX_empty_constructor()
  {
    el = EventList();
    //Generate the histrogram bins
    MantidVec shared_x;
    double tof; //in ns
    for (tof=0; tof<16e3*1e3; tof += 1e4)
    {
      //bins of 10 microsec
      shared_x.push_back(tof);
    }
    el.setX(shared_x);
    //Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.dataX()==shared_x);
  }

  void test_empty_histogram()
  {
    //Make sure there's no data
    el.clear();
    const EventList el2(el);

    //Getting data before setting X returns empty vector
    TS_ASSERT_EQUALS(el2.dataY()->size(), 0);

    //Now do set up an X axis.
    this->test_setX();
    MantidVec X, Y;
    const EventList el3(el);
    X = el3.dataX();
    Y = *el3.dataY();
    //Histogram is 0, since I cleared all the events
    for (int i=0; i<X.size()-1; i++)
    {
      TS_ASSERT_EQUALS(Y[i], 0);
    }
  }

  void test_no_histogram_x()
  {
    //Make sure there's no data
    el.clear();
    //Now give it some fake data, with NUMEVENTS events in it.
    this->fake_data();
    const EventList el4(el);
    TS_ASSERT_EQUALS(el4.dataY()->size(), 0);
  }

  void fake_uniform_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (double tof=100; tof < 20e6; tof += 5000)
    {
      //tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      el += TofEvent( tof, rand()%1000);
    }
  }

  void fake_uniform_time_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (double time=0; time < 1000; time++)
    {
      //All pulse times from 0 to 999
      el += TofEvent( rand()%1000, time);
    }
  }

  void test_histogram()
  {
    this->fake_uniform_data();
    this->test_setX(); //Set it up
    MantidVec X, Y;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    //The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 2.0);
    }

  }

  void test_histogram_with_first_bin_higher_than_first_event()
  {
    //Make sure the algorithm handles it if the first bin > then the first event tof
    this->fake_uniform_data();

    //Generate the histrogram bins starting at 1000
    MantidVec shared_x;
    for (double tof=1000; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA)
      shared_x.push_back(tof);
    el.setX(shared_x);

    //Get them back
    MantidVec X, Y;
    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);

    //The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (int i=0; i<Y.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 2.0);
    }

  }

  void test_random_histogram()
  {
    this->fake_data();
    this->test_setX();
    MantidVec X, Y;
    const EventList el3(el);
    X = el3.dataX();
    Y = *el3.dataY();
    TS_ASSERT_EQUALS(Y.size(), X.size()-1);
    for (int i=0; i<X.size()-1; i++)
    {
      //No data was generated above 10 ms.
      if (X[i] > 10e6)
        TS_ASSERT_EQUALS(Y[i], 0.0);
    }
  }


  void test_histogram_const_call()
  {
    this->fake_uniform_data();
    this->test_setX(); //Set it up WITH THE default binning
    //Ok, we have this many bins
    TS_ASSERT_EQUALS(this->el.getRefX()->size(), NUMBINS+1);

    //Make one with half the bins
    MantidVec some_other_x;
    double tof; //in ns
    for (tof=0; tof<BIN_DELTA*(NUMBINS+1); tof += BIN_DELTA*2)
      some_other_x.push_back(tof);

    const EventList el3(el); //need to copy to a const method in order to access the data directly.
    MantidVec Y, E;
    el3.generateCountsHistogram(some_other_x, Y);
    el3.generateErrorsHistogram(Y, E);
    TS_ASSERT_EQUALS(Y.size(), some_other_x.size()-1);
    TS_ASSERT_EQUALS(E.size(), some_other_x.size()-1);
    //Now there are 4 events per bin
    for (int i=0; i<Y.size(); i++)
      TS_ASSERT_EQUALS(Y[i], 4.0);

    //With all this jazz, the original element is unchanged
    TS_ASSERT_EQUALS(this->el.getRefX()->size(), NUMBINS+1);

  }


  void test_convertTof()
  {
    this->fake_uniform_data();
    size_t old_num = this->el.getEvents().size();

    //Do convert
    this->el.convertTof( 2.5 );
    //Unchanged size
    TS_ASSERT_EQUALS(old_num, this->el.getEvents().size());

    //Original tofs were 100, 5100, 10100, etc.)
    TS_ASSERT_EQUALS(this->el.getEvents()[0].tof(), 250.0);
    TS_ASSERT_EQUALS(this->el.getEvents()[1].tof(), 12750.0);

  }


  void testMaskTOF()
  {
    //tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
    this->fake_uniform_data();
    //Start with 4000 events
    TS_ASSERT_EQUALS( el.getNumberEvents(), 4000);
    //Mask out 5-10 milliseconds
    el.maskTof( 5e6, 10e6);
    vector<TofEvent> rel = el.getEvents();
    int i;
    for (i=0; i<rel.size(); i++)
    {
      //No tofs in that range
      TS_ASSERT((rel[i].tof() < 5e6) || (rel[i].tof() > 10e6));
    }
    TS_ASSERT_EQUALS( el.getNumberEvents(), 3000);
  }

  void testSetTofs()
  {
    this->fake_data();

    // Grab original data as it will become "new" data
    MantidVec T;
    T = *el.getTofs();

    // Convert to make values something different
    this->el.convertTof(4.0, 2.0);
    double old_value = this->el.getEvents()[0].tof();
    size_t old_size = this->el.getEvents().size();

    // Set "new" data
    this->el.setTofs(T);
    double new_value = this->el.getEvents()[0].tof();
    size_t new_size = this->el.getEvents().size();

    TS_ASSERT_EQUALS(old_size, new_size);
    TS_ASSERT_DIFFERS(old_value, new_value);
  }


  void testFilterByPulseTime()
  {
    this->fake_data();

    //Filter into this
    EventList out = EventList();
    el.filterByPulseTime(100, 200, out);

    std::vector<TofEvent> eventsIn = el.getEvents();
    int numGood = 0;
    for (int i=0; i < eventsIn.size(); i++)
      if ((eventsIn[i].pulseTime() >= 100) && (eventsIn[i].pulseTime() < 200))
        numGood++;

    //Good # of events.
    TS_ASSERT_EQUALS( numGood, out.getNumberEvents());

    std::vector<TofEvent> events = out.getEvents();
    for (int i=0; i < events.size(); i++)
    {
      //Check that the times are within the given limits.
      TS_ASSERT_LESS_THAN_EQUALS( 100, events[i].pulseTime());
      TS_ASSERT_LESS_THAN( events[i].pulseTime(), 200);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void testSplit()
  {
    this->fake_uniform_time_data();

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    //Start only at 100
    for (int i=1; i<10; i++)
    {
      //Reject the odd hundreds pulse times (100-199, 300-399, etc).
      if ((i%2) == 0)
        split.push_back( std::pair<PulseTimeType, int>(i*100, i) );
      else
        split.push_back( std::pair<PulseTimeType, int>(i*100, -1) );
    }

    //Do the splitting
    el.splitByTime(split, outputs);

    //No events in the first ouput 0-99
    TS_ASSERT_EQUALS( outputs[0]->getNumberEvents(), 0);

    for (int i=1; i<10; i++)
    {
      EventList * myOut = outputs[i];
      //std::cout << i << " " << myOut->getNumberEvents() << "\n";
      if ((i%2) == 0)
      {
        //Even
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 100);
      }
      else
      {
        //Odd
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 0);
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  // Splitter should handle the splitter ending before the event list
  void testSplitterIsSmallerThanEventList()
  {
    this->fake_uniform_time_data();

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    //Keep only from 200 to 300
    split.push_back( std::pair<PulseTimeType, int>(200, 5) );
    split.push_back( std::pair<PulseTimeType, int>(300, -1) );

    //Do the splitting
    el.splitByTime(split, outputs);

    for (int i=0; i<10; i++)
    {
      EventList * myOut = outputs[i];
      if (i==5)
      {
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 100);
      }
      else
      {
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 0);
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  // Splitter should handle the events running out before the end of events
  void testSplitterIsBiggerThanEventList()
  {
    this->fake_uniform_time_data();

    std::vector< EventList * > outputs;
    for (int i=0; i<10; i++)
      outputs.push_back( new EventList() );

    TimeSplitterType split;
    split.push_back( std::pair<PulseTimeType, int>(-1000, 5) );
    split.push_back( std::pair<PulseTimeType, int>(3300, -1) );

    //Do the splitting
    el.splitByTime(split, outputs);

    for (int i=0; i<10; i++)
    {
      EventList * myOut = outputs[i];
      if (i==5)
      {
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 1000);
      }
      else
      {
        TS_ASSERT_EQUALS( myOut->getNumberEvents(), 0);
      }
    }
  }
};




#endif /// EVENTLISTTEST_H_


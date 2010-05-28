#ifndef EVENTLISTTEST_H_
#define EVENTLISTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

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
    TS_ASSERT_EQUALS(e.frame(), 456);
  }

  void testAssign()
  {
    TofEvent e2;
    e2 = e;
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.frame(), 456);
  }

  void testConstructors()
  {
    TofEvent e2 = TofEvent(e);
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.frame(), 456);

    TofEvent e3 = TofEvent(890.234, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890.234);
    TS_ASSERT_EQUALS(e3.frame(), 321);
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
//    TS_ASSERT_EQUALS(e.frame(), 0);
//  }
};


//==========================================================================================
class EventListTest : public CxxTest::TestSuite
{
private:
  EventList el;

public:
  EventListTest()
  {
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
    TS_ASSERT_EQUALS(rel[0].frame(), 200);
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

  //==================================================================================
  //--- Sorting Tests ---

  void fake_data()
  {
    //Clear the list
    el = EventList();
    //Create some mostly-reasonable fake data.
    srand(1234); //Fixed random seed
    for (int i=0; i < 100; i++)
    {
      //Random tof up to 10 ms
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

  void testSortFrame()
  {
    el.sortFrame();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].frame(), 60);
    TS_ASSERT_EQUALS(rel[1].frame(), 200);
    TS_ASSERT_EQUALS(rel[2].frame(), 400);

    this->fake_data();
    el.sort(FRAME_SORT);
    rel = el.getEvents();
    int i;
    for (i=1; i<100; i++)
    {
      TS_ASSERT_LESS_THAN_EQUALS(rel[i-1].frame(), rel[i].frame());
    }
  }


  //==================================================================================
  //--- Histogramming Tests ---

  void test_setX()
  {
    //Generate the histrogram bins
    EventList::StorageType shared_x;
    double tof; //in ns
    for (tof=0; tof<16e3*1e3; tof += 1e4)
    {
      //bins of 10 microsec
      shared_x.push_back(tof);
    }
    el.setX(shared_x);
    //Do we have the same data in X?
    TS_ASSERT(el.dataX() == shared_x);
  }

  void test_empty_histogram()
  {
    el = EventList(); //Clear any events

    //Getting data before setting X throws
    TS_ASSERT_THROWS(el.dataY(), runtime_error);

    this->test_setX(); //Set it up
    EventList::StorageType X, Y;
    X = el.dataX();
    Y = el.dataY();
    for (int i=0; i<X.size(); i++)
    {
      TS_ASSERT_EQUALS(Y[i], 0);
    }
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

  void test_histogram()
  {
    this->fake_uniform_data();
    this->test_setX(); //Set it up
    EventList::StorageType X, Y;
    X = el.dataX();
    Y = el.dataY();
    //The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (int i=0; i<Y.size()-1; i++)
    {
      if (Y[i] != 2) std::cout << "Different at i=" << i << "\n";
      TS_ASSERT_EQUALS(Y[i], 2.0);
    }
  }

  void test_random_histogram()
  {
    this->fake_data();
    this->test_setX();
    EventList::StorageType X, Y;
    X = el.dataX();
    Y = el.dataY();
    for (int i=0; i<X.size()-1; i++)
    {
      //No data was generated above 10 ms.
      if (X[i] > 10e6)
        TS_ASSERT_EQUALS(Y[i], 0.0);
    }
  }


  //==================================================================================
  //--- Unit unittests ---

  void test_units()
  {
    //TODO!
  }


};




#endif /// EVENTLISTTEST_H_


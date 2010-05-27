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
  }

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


//  void test_readX()
//  {
//    this->fake_data();
//    vector<double>x = el.readX();
//  }
};




#endif /// EVENTLISTTEST_H_


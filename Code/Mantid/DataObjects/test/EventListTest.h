#ifndef EVENTLISTTEST_H_
#define EVENTLISTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"

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

    TofEvent e3 = TofEvent(890, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890);
    TS_ASSERT_EQUALS(e3.frame(), 321);
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
    vector<TofEvent> mylist;
    mylist.push_back(TofEvent(100,200));
    mylist.push_back(TofEvent(3,4));
    mylist.push_back(TofEvent(50,60));
    el = EventList(mylist);
  }

  void testInit()
  {
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
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

    el += TofEvent(999, 888);
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 7);
  }

};




#endif /// EVENTLISTTEST_H_


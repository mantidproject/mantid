#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDBoxTest :    public CxxTest::TestSuite
{

public:
  void testConstructor()
  {
    MDBox<3> b3;
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);

//    std::cout << sizeof(b3) << "\n";
//    std::cout << sizeof(MDBox<0> ) << "\n";
//    std::cout << sizeof(MDBox<1> ) << "\n";
//    std::vector< int > v;
//    std::cout << sizeof(v) << "\n";
  }

  void testAddPoint()
  {
    MDBox<2> b;
    MDEvent<2> ev(1.2, 3.4);
    ev.setCoord(0, 2.0);
    ev.setCoord(1, 3.0);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 1)
  }

  void testClear()
  {
    MDBox<2> b;
    MDEvent<2> ev(1.2, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 2)
    TS_ASSERT_DELTA( b.getSignal(), 2.4, 1e-5)
    b.clear();
    TS_ASSERT_EQUALS( b.getNPoints(), 0)
    TS_ASSERT_DELTA( b.getSignal(), 0.0, 1e-5)
    TS_ASSERT_DELTA( b.getErrorSquared(), 0.0, 1e-5)
  }

  void test_getPoints()
  {
    MDBox<2> b;
    MDEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getPoints().size(), 3);
    TS_ASSERT_EQUALS( b.getPoints()[2].getSignal(), 4.0);
  }

  void test_sptr()
  {
    MDBox<3>::sptr a( new MDBox<3>());
    //TS_ASSERT_EQUALS( sizeof(a), 16);

  }
};

#endif

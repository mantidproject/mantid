#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MDDataObjects/Events/MDPoint.h"
#include "MDDataObjects/Events/MDBox.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDDataObjects;

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
    MDPoint<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addPoint(ev);
  }


  void testStats()
  {
    MDBox<2> b;
    MDPoint<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addPoint(ev);

    // Now check the stats
    TS_ASSERT_EQUALS(b.getStats(0).min, 2.0);
    TS_ASSERT_EQUALS(b.getStats(0).max, 2.0);
    TS_ASSERT_EQUALS(b.getStats(0).total, 2.0);
    TS_ASSERT_EQUALS(b.getStats(0).approxVariance, 0.0);

    // Add another point.
    ev.setCenter(0, 4.0);
    ev.setCenter(1, -3.0);
    b.addPoint(ev);
    TS_ASSERT_EQUALS(b.getStats(0).min, 2.0);
    TS_ASSERT_EQUALS(b.getStats(0).max, 4.0);
    TS_ASSERT_EQUALS(b.getStats(0).total, 6.0);
    TS_ASSERT_EQUALS(b.getStats(0).approxVariance, 1.0);
    // And the other dimension
    TS_ASSERT_EQUALS(b.getStats(1).min, -3.0);
    TS_ASSERT_EQUALS(b.getStats(1).max, 3.0);
    TS_ASSERT_EQUALS(b.getStats(1).total, 0.0);
    TS_ASSERT_EQUALS(b.getStats(1).approxVariance, 9.0);

    // Signal and error get summed
    TS_ASSERT_DELTA( b.getSignal(), 2.4, 1e-6);
    TS_ASSERT_DELTA( b.getErrorSquared(), 6.8, 1e-6);
  }

  void test_getPoints()
  {
    MDBox<2> b;
    MDPoint<2> ev(4.0, 3.4);
    b.addPoint(ev);
    b.addPoint(ev);
    b.addPoint(ev);
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

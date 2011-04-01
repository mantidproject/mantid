#ifndef BOXPLITCONTROLLER_TEST_H
#define BOXPLITCONTROLLER_TEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/BoxController.h"
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class BoxControllerTest :    public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    BoxController sc(2);
    TS_ASSERT_EQUALS( sc.getNDims(), 2);
  }

  void test_willSplit()
  {
    BoxController sc(2);
    sc.setMaxDepth(4);
    sc.setSplitThreshold(10);
    TS_ASSERT( sc.willSplit(100,3) );
    TS_ASSERT( !sc.willSplit(100,4) );
    TS_ASSERT( !sc.willSplit(2,3) );
    TS_ASSERT( !sc.willSplit(100,5) );
  }

  void test_getSplitInto()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 1000);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);
    sc.setSplitInto(1,5);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 500);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 5);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);

  }

  void test_maxDepth()
  {
    BoxController sc(3);
    sc.setMaxDepth(12);
    TS_ASSERT_EQUALS( sc.getMaxDepth(), 12);
  }

  void test_trackNumBoxes()
  {
    BoxController sc(3);
    sc.setMaxDepth(4);
    sc.setSplitInto(10);
    const std::vector<size_t> & num = sc.getNumMDBoxes();
    TS_ASSERT_EQUALS( num.size(), 5);
    TS_ASSERT_EQUALS( num[0], 1);
    TS_ASSERT_EQUALS( num[1], 0);

    sc.trackNumBoxes(0);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 1000);

    sc.trackNumBoxes(1);
    sc.trackNumBoxes(1);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 998);
    TS_ASSERT_EQUALS( num[2], 2000);

  }




};

#endif

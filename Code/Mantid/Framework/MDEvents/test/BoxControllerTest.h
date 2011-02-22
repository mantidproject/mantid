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
    sc.setSplitThreshold(10);
    TS_ASSERT( sc.willSplit(5,10) );
    TS_ASSERT( !sc.willSplit(2,3) );
  }

  void test_splitInto()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.splitInto(0), 10);
    TS_ASSERT_EQUALS( sc.splitInto(1), 10);
    TS_ASSERT_EQUALS( sc.splitInto(2), 10);
    sc.setSplitInto(1,5);
    TS_ASSERT_EQUALS( sc.splitInto(0), 10);
    TS_ASSERT_EQUALS( sc.splitInto(1), 5);
    TS_ASSERT_EQUALS( sc.splitInto(2), 10);
  }


};

#endif

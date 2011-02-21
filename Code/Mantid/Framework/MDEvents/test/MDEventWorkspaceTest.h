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
    BoxController sc(10);
  }

  void test_willSplit()
  {
    BoxController sc(10);
    TS_ASSERT( sc.willSplit(5,10) );
    TS_ASSERT( !sc.willSplit(2,3) );
  }


};

#endif

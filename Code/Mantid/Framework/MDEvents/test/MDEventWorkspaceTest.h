#ifndef BOXPLITCONTROLLER_TEST_H
#define BOXPLITCONTROLLER_TEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/BoxSplitController.h"
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class BoxSplitControllerTest :    public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    BoxSplitController sc(10);
  }

  void test_willSplit()
  {
    BoxSplitController sc(10);
    TS_ASSERT( sc.willSplit(5,10) );
    TS_ASSERT( !sc.willSplit(2,3) );
  }


};

#endif

#ifndef MDEVENTWORKSPACETEST_H
#define MDEVENTWORKSPACETEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvent/MDEvent.h"
#include "MantidMDEvent/MDEventWorkspace.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDDataObjects;

class MDEventWorkspaceTest :    public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    MDEventWorkspace<3> ew3;
    TS_ASSERT_EQUALS( ew3.getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3.getNPoints(), 0);
  }


};

#endif

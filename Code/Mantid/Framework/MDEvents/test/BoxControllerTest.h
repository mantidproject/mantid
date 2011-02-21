#ifndef MDEVENTWORKSPACETEST_H
#define MDEVENTWORKSPACETEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDEventWorkspaceTest :    public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    MDEventWorkspace<MDEvent<3>, 3> ew3;
    TS_ASSERT_EQUALS( ew3.getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3.getNPoints(), 0);
  }

  void test_Constructor_IMDEventWorkspace()
  {
    IMDEventWorkspace * ew3 = new MDEventWorkspace<MDEvent<3>, 3>();
    TS_ASSERT_EQUALS( ew3->getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3->getNPoints(), 0);
    delete ew3;
  }


};

#endif
